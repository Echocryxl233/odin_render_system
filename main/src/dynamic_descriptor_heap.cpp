#include "command_list_manager.h"
#include "dynamic_descriptor_heap.h"
#include "graphics_core.h"

using namespace std;
using Microsoft::WRL::ComPtr;

vector<ComPtr<ID3D12DescriptorHeap>> DynamicDescriptorHeap::descriptor_heap_pool_[2];
queue<ID3D12DescriptorHeap*> DynamicDescriptorHeap::avalible_descriptor_heaps_[2];
queue<pair<uint64_t, ID3D12DescriptorHeap*>> DynamicDescriptorHeap::retired_descriptor_heaps_[2];

DynamicDescriptorHeap::DynamicDescriptorHeap(CommandContext& owning_context, D3D12_DESCRIPTOR_HEAP_TYPE type) 
    : owning_context_(owning_context) , heap_type_(type)
    {
  current_heap_ptr_ = nullptr;
  current_offset_ = 0;
  descriptor_size_ = Graphics::Core.Device()->GetDescriptorHandleIncrementSize(type);
}

ID3D12DescriptorHeap* DynamicDescriptorHeap::RequestDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type) {
  int type_index = type == D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER ? 1 : 0;
  
  auto& retired_heaps = retired_descriptor_heaps_[type_index];
  auto& avalible_heaps = avalible_descriptor_heaps_[type_index];

  while (!retired_heaps.empty() && CommandListManager::Instance().IsFenceComplete(retired_heaps.front().first)) {
    avalible_heaps.push(retired_heaps.front().second);
    retired_heaps.pop();    
  }
    
  if (!avalible_heaps.empty()) {
    ID3D12DescriptorHeap* ret = avalible_heaps.front();
    avalible_heaps.pop();
    return ret;
  }
  else {
    D3D12_DESCRIPTOR_HEAP_DESC heap_desc;
    heap_desc.Type = type;
    heap_desc.NumDescriptors = kNumDescriptorsPerHeap;
    //  heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAGS::D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAGS::D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    
  //  D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
    heap_desc.NodeMask = 0;

    ComPtr<ID3D12DescriptorHeap> heap;
    ASSERT_SUCCESSED(Graphics::Core.Device()->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(heap.GetAddressOf())));
    wstring name(L"Descriptor Heap No.");
    name += to_wstring(descriptor_heap_pool_->size());
    heap->SetName(name.c_str());
    descriptor_heap_pool_[type_index].push_back(heap);

    return heap.Get();
  }
}

void DynamicDescriptorHeap::DiscardDescriptorHeaps(D3D12_DESCRIPTOR_HEAP_TYPE type, uint64_t fence_value, 
    const vector<ID3D12DescriptorHeap*>& used_descriptor_heaps) {
  int type_index = type == D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER ? 1 : 0;

  auto& retired_heaps = retired_descriptor_heaps_[type_index];
  for (auto it : used_descriptor_heaps) {
    auto pair = make_pair(fence_value, it);
    retired_heaps.push(pair);
  }
}

void DynamicDescriptorHeap::SetGraphicsDescriptorHandles(UINT root_index, UINT offset, UINT count, D3D12_CPU_DESCRIPTOR_HANDLE handles[]) {
  graphics_handle_cache_.StageTableHandles(root_index, offset, count, handles);
}

void DynamicDescriptorHeap::SetComputeDescriptorHandles(UINT root_index, UINT offset, UINT count, D3D12_CPU_DESCRIPTOR_HANDLE handles[]) {
  compute_handle_cache_.StageTableHandles(root_index, offset, count, handles);
}

void DynamicDescriptorHeap::ClearupUsedHeaps(uint64_t fence_value) {
  RetireCurrentHeaps();
  RetireUsedHeap(fence_value);
  graphics_handle_cache_.ClearCache();
  compute_handle_cache_.ClearCache();
}


void DynamicDescriptorHeap::CopyAndBindStagedTables(DescriptorHandleCache& handle_cache,
    ID3D12GraphicsCommandList* command_list,
    void (STDMETHODCALLTYPE ID3D12GraphicsCommandList::* set_function)(UINT, D3D12_GPU_DESCRIPTOR_HANDLE)) {
  uint32_t need_size = handle_cache.ComputeStagedSize();
  if (!HasSpace(need_size)) {
    RetireCurrentHeaps();
    UnbindAllValid();
    need_size = handle_cache.ComputeStagedSize();
  }

  owning_context_.SetDescriptorHeap(heap_type_, GetHeapPoint());
  handle_cache.CopyAndBindStaleTables(heap_type_, Allocate(need_size), descriptor_size_, 
    command_list, set_function);
};

void DynamicDescriptorHeap::UnbindAllValid() {
  graphics_handle_cache_.UnbindValidTable();
  compute_handle_cache_.UnbindValidTable();
}

ID3D12DescriptorHeap* DynamicDescriptorHeap::GetHeapPoint() {
  if (current_heap_ptr_ == nullptr) {
    current_heap_ptr_ = RequestDescriptorHeap(heap_type_);
    first_handle_ = DescriptorHandle(
        current_heap_ptr_->GetCPUDescriptorHandleForHeapStart(),
        current_heap_ptr_->GetGPUDescriptorHandleForHeapStart());
  }

  return current_heap_ptr_;
}

uint32_t DynamicDescriptorHeap::DescriptorHandleCache::ComputeStagedSize() {
  uint32_t root_param = stale_root_descriptor_table_bitmap;
  uint32_t root_index;
  uint32_t need_space = 0;
  while (_BitScanForward((unsigned long*)&root_index, root_param)) {
    root_param ^= (1 << root_index);

    uint32_t bitmap = root_descriptor_tables[root_index].assigned_handles_bitmap;

    uint32_t max_staled_size;
    assert(_BitScanReverse((unsigned long*)&max_staled_size, bitmap)
      && "Root entry has marked as stale but has no staled descriptor" 
      && "maybe you forgot to set root signature");

    need_space += max_staled_size + 1;
  }

  return need_space;
}

void DynamicDescriptorHeap::DescriptorHandleCache::StageTableHandles(
      UINT root_index, UINT offset, UINT descriptor_count, 
      D3D12_CPU_DESCRIPTOR_HANDLE handles[]) {
  assert((stale_root_descriptor_table_bitmap | (1 << root_index)) != 0 && "Root table has been staled"); 
  assert(offset + descriptor_count <= root_descriptor_tables[root_index].table_size && 
      "DescriptorTableCache Overflow, check the 'handle count' and 'table size' ");
  
  DescriptorTableCache& table = root_descriptor_tables[root_index];
  D3D12_CPU_DESCRIPTOR_HANDLE* dest_handle = table.Start + offset;
  for (UINT i=0; i<descriptor_count; ++i) {
    dest_handle[i] = handles[i];
  }
  table.assigned_handles_bitmap |= (((1 << descriptor_count) - 1)<< offset);
  stale_root_descriptor_table_bitmap |= 1 << root_index;
}

//  parse the root signature, and get the table size and start handle
void DynamicDescriptorHeap::DescriptorHandleCache::ParseRootSignature(
    D3D12_DESCRIPTOR_HEAP_TYPE type, const RootSignature& root_signature) {
  UINT current_offset = 0;
  stale_root_descriptor_table_bitmap = 0;

  root_descriptor_tables_bitmap = type == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER ?   
    root_signature.sampler_table_bit_map_ : root_signature.descriptor_table_bit_map_;

  unsigned long table_param = root_descriptor_tables_bitmap;
  unsigned long root_index = 0;
  while (_BitScanForward(&root_index, table_param)) {
    table_param ^= (1 << root_index);

    UINT table_size = root_signature.descriptor_sizes[root_index];
    assert(table_size > 0);
      
    DescriptorTableCache& table_cache = root_descriptor_tables[root_index];
    table_cache.Start = cache_handles + current_offset;
    table_cache.table_size = table_size;

    current_offset += table_size;
  }
  max_cache_descriptors = current_offset;
  assert((max_cache_descriptors <= kMaxDescriptorCount) && "Exceeded user-supplied maximum cache size");
}

void DynamicDescriptorHeap::DescriptorHandleCache::UnbindValidTable() {
  stale_root_descriptor_table_bitmap = 0;

  uint32_t root_param = root_descriptor_tables_bitmap;
  uint32_t root_index = 0;
  while(_BitScanForward((unsigned long*)&root_index, root_param)) {
    root_param ^= 1 << root_index;

    if (root_descriptor_tables[root_index].assigned_handles_bitmap != 0) {
      stale_root_descriptor_table_bitmap |= (1<<root_index);
    }
  }
}

void DynamicDescriptorHeap::DescriptorHandleCache::CopyAndBindStaleTables(D3D12_DESCRIPTOR_HEAP_TYPE type,
    DescriptorHandle start_handle, uint32_t descriptor_size, ID3D12GraphicsCommandList* command_list,
    void (STDMETHODCALLTYPE ID3D12GraphicsCommandList::* set_function)(UINT, D3D12_GPU_DESCRIPTOR_HANDLE)) {

  UINT root_table_indice[DynamicDescriptorHeap::DescriptorHandleCache::kMaxDescriptorCount];
  uint32_t root_table_size[DynamicDescriptorHeap::DescriptorHandleCache::kMaxDescriptorCount];
  uint32_t root_index = 0;
  uint32_t root_bit_map = stale_root_descriptor_table_bitmap;
  uint32_t root_table_count = 0;  //  how many table will be copy
  uint32_t need_space = 0;

  //  extract the evety root index, and get how many table will be copy
  while (_BitScanForward((unsigned long*)&root_index, root_bit_map)) {
    root_table_indice[root_table_count] = root_index;
    root_bit_map ^= (1 << root_index);

    uint32_t max_staled_size;
    assert(_BitScanReverse((unsigned long*)&max_staled_size, root_descriptor_tables[root_index].assigned_handles_bitmap)
      && "Root entry has marked as stale but has no staled descriptor");

    need_space += max_staled_size + 1;
    root_table_size[root_table_count] = max_staled_size + 1;

    root_table_count++;
  }   

  stale_root_descriptor_table_bitmap = 0; //  reset the bit map
  
  const uint32_t kMaxCount = 16;

  UINT dest_descriptor_range = 0;
  D3D12_CPU_DESCRIPTOR_HANDLE dest_descriptor_range_starts[kMaxCount];
  UINT dest_descriptor_range_sizes[kMaxCount];

  UINT src_descriptor_range = 0;
  D3D12_CPU_DESCRIPTOR_HANDLE src_descriptor_range_starts[kMaxCount];
  UINT src_descriptor_range_sizes[kMaxCount];

  unsigned long descriptor_count_check;
  int over_max_count = 0;

  for (uint32_t i=0; i< root_table_count; ++i) {
    root_index = root_table_indice[i];
    //  command_list->SetGraphicsRootDescriptorTable(root_index, start_handle.GpuHandle());
    (command_list->*set_function)(root_index, start_handle.GpuHandle());

    DescriptorTableCache& root_table = root_descriptor_tables[root_index];
    D3D12_CPU_DESCRIPTOR_HANDLE* src_handle = root_table.Start;
    uint64_t mask_bit_map = (uint64_t)root_table.assigned_handles_bitmap;
    
    D3D12_CPU_DESCRIPTOR_HANDLE current_dest = start_handle.CpuHandle();

    start_handle += root_table_size[i] * descriptor_size;

    unsigned long skip_count;

    while (_BitScanForward64(&skip_count, mask_bit_map)) {
      mask_bit_map >>= skip_count;
      src_handle += skip_count;
      current_dest.ptr += skip_count * descriptor_size;

      unsigned long descriptor_count;
      _BitScanForward64(&descriptor_count, ~mask_bit_map);
      descriptor_count_check = descriptor_count;
      mask_bit_map >>= descriptor_count;

      if (descriptor_count + src_descriptor_range > kMaxCount) {
        Graphics::Core.Device()->CopyDescriptors(dest_descriptor_range, dest_descriptor_range_starts, dest_descriptor_range_sizes,
          src_descriptor_range, src_descriptor_range_starts, src_descriptor_range_sizes, type);

        src_descriptor_range = 0;
        dest_descriptor_range = 0;
        ++over_max_count;
      }

      dest_descriptor_range_starts[dest_descriptor_range] = current_dest;
      dest_descriptor_range_sizes[dest_descriptor_range] = descriptor_count;
      ++dest_descriptor_range;
      
      for (uint32_t j=0;j< descriptor_count; ++j) {
        src_descriptor_range_starts[src_descriptor_range] = src_handle[j];
        src_descriptor_range_sizes[src_descriptor_range] = 1;
        ++src_descriptor_range;
      }
    
      src_handle += descriptor_count;
      current_dest.ptr += descriptor_count * descriptor_size;
    }
  }

  Graphics::Core.Device()->CopyDescriptors(dest_descriptor_range, dest_descriptor_range_starts, dest_descriptor_range_sizes,
    src_descriptor_range, src_descriptor_range_starts, src_descriptor_range_sizes, type);
}

void DynamicDescriptorHeap::RetireUsedHeap(uint64_t fence_value) {
  DiscardDescriptorHeaps(heap_type_, fence_value, retired_heaps_);
  retired_heaps_.clear();
}

void DynamicDescriptorHeap::RetireCurrentHeaps() {
  if (current_offset_ == 0) {
    assert(current_heap_ptr_ == nullptr);
    return;
  }

  retired_heaps_.push_back(current_heap_ptr_);
  current_heap_ptr_ = nullptr;
  current_offset_ = 0;
}


