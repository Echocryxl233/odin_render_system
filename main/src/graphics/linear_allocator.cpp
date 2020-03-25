#include "linear_allocator.h"
#include "base/common_helper.h"
#include "graphics/command_list_manager.h"
#include "graphics/graphics_core.h"


using namespace std;
using namespace Graphics;

LinearAllocatorPageManager LinearAllocator::page_managers_[2];

LinearAllocatorType LinearAllocatorPageManager::auto_type_ = kGpuExclusive;

LinearAllocatorPageManager::LinearAllocatorPageManager() {
  allocator_type_ = auto_type_;
  auto_type_ = (LinearAllocatorType)(auto_type_ + 1);
  assert(auto_type_ <= kNumAllocatorTypes);
}

LinearAllocationPage* LinearAllocatorPageManager::RequestPage() {

  while (!retired_pages_.empty() &&
      CommandListManager::Instance().IsFenceComplete(retired_pages_.front().first)) {
    avalible_pages_.push(retired_pages_.front().second);
    retired_pages_.pop();
  }

  LinearAllocationPage* page = nullptr;

  if (!avalible_pages_.empty()) {
    page = avalible_pages_.front();
    avalible_pages_.pop();
  }
  else {
    page = CreateNewPage();
    page_pool_.emplace_back(page);
  }

  return page;
}

LinearAllocationPage* LinearAllocatorPageManager::CreateNewPage(size_t page_size)
{
  D3D12_HEAP_PROPERTIES heap_props;
  heap_props.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
  heap_props.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
  heap_props.CreationNodeMask = 1;
  heap_props.VisibleNodeMask = 1;

  D3D12_RESOURCE_DESC resource_desc;
  resource_desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
  resource_desc.Alignment = 0;
  resource_desc.Height = 1;
  resource_desc.DepthOrArraySize = 1;
  resource_desc.MipLevels = 1;
  resource_desc.Format = DXGI_FORMAT_UNKNOWN;
  resource_desc.SampleDesc.Count = 1;
  resource_desc.SampleDesc.Quality = 0;
  resource_desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

  D3D12_RESOURCE_STATES default_usage;

  if (allocator_type_ == kGpuExclusive)
  {
    heap_props.Type = D3D12_HEAP_TYPE_DEFAULT;
    resource_desc.Width = page_size == 0 ? kGpuAllocatorPageSize : page_size;
    resource_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    default_usage = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
  }
  else
  {
    heap_props.Type = D3D12_HEAP_TYPE_UPLOAD;
    resource_desc.Width = page_size == 0 ? kCpuAllocatorPageSize : page_size;
    resource_desc.Flags = D3D12_RESOURCE_FLAG_NONE;
    default_usage = D3D12_RESOURCE_STATE_GENERIC_READ;
  }

  ID3D12Resource* buffer;
  ASSERT_SUCCESSED(Core.Device()->CreateCommittedResource(&heap_props, D3D12_HEAP_FLAG_NONE,
    &resource_desc, default_usage, nullptr, IID_PPV_ARGS(&buffer)));

  buffer->SetName(L"LinearAllocator Page");

  return new LinearAllocationPage(buffer, default_usage);
}


void LinearAllocatorPageManager::DiscardPages(uint64_t fence_value, const vector<LinearAllocationPage*> pages) {
  for (auto it = pages.begin();it != pages.end(); ++it) {
    retired_pages_.push(make_pair(fence_value, (*it)));
  }
}

void LinearAllocatorPageManager::FreeLargePages(uint64_t fence_value, const vector<LinearAllocationPage*> pages) {
  while (!deleted_pages_.empty() &&
    CommandListManager::Instance().IsFenceComplete(deleted_pages_.front().first)) {
    delete deleted_pages_.front().second;
    deleted_pages_.pop();
  }

  for (auto it = pages.begin(); it != pages.end(); ++it) {
    (*it)->Unmap();
    deleted_pages_.push(make_pair(fence_value, (*it)));
  }
}

LinearAllocator::LinearAllocator(LinearAllocatorType type) 
    : allocation_type_(type), page_offset_(~(size_t)0), 
      page_size_(0), current_page_(nullptr) {

  assert(type > kInvalidAllocator && type < kNumAllocatorTypes);
  page_size_ = (type == kGpuExclusive ? kGpuAllocatorPageSize : kCpuAllocatorPageSize);
}

void LinearAllocator::CleanupUsedPages(uint64_t FenceID)
{
  if (current_page_ == nullptr)
    return;

  retired_pages_.push_back(current_page_);
  current_page_ = nullptr;
  page_offset_ = 0;

  page_managers_[allocation_type_].DiscardPages(FenceID, retired_pages_);
  retired_pages_.clear();

  page_managers_[allocation_type_].FreeLargePages(FenceID, large_page_list_);
  large_page_list_.clear();
}

DynAlloc LinearAllocator::AllocateLargePage(size_t size_in_bytes)
{
  LinearAllocationPage* one_off = page_managers_[allocation_type_].CreateNewPage(size_in_bytes);
  large_page_list_.push_back(one_off);

  DynAlloc ret(*one_off, 0, size_in_bytes);
  ret.Data = one_off->CpuAddress;
  ret.GpuAddress = one_off->GpuAddress;

  return ret;
}

DynAlloc LinearAllocator::Allocate(size_t SizeInBytes, size_t Alignment)
{
  const size_t AlignmentMask = Alignment - 1;

  // Assert that it's a power of two.
  assert((AlignmentMask & Alignment) == 0);

  // Align the allocation
  const size_t AlignedSize = Math::AlignUpWithMask(SizeInBytes, AlignmentMask);

  if (AlignedSize > page_size_)
    return AllocateLargePage(AlignedSize);

  page_offset_ = Math::AlignUp(page_offset_, Alignment);

  if (page_offset_ + AlignedSize > page_size_)
  {
    assert(current_page_ != nullptr);
    retired_pages_.push_back(current_page_);
    current_page_ = nullptr;
  }

  if (current_page_ == nullptr)
  {
    current_page_ = page_managers_[allocation_type_].RequestPage();
    page_offset_ = 0;
  }

  DynAlloc ret(*current_page_, page_offset_, AlignedSize);
  ret.Data = (uint8_t*)current_page_->CpuAddress + page_offset_;
  ret.GpuAddress = current_page_->GpuAddress + page_offset_;

  page_offset_ += AlignedSize;

  return ret;
}