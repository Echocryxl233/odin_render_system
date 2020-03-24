#pragma once

#ifndef DYNAMICDESCRIPTORHEAP_H
#define DYNAMICDESCRIPTORHEAP_H

#include "utility.h"
//  #include "graphics/command_context.h"
#include "graphics/descriptor_allocator.h"
#include "graphics/root_signature.h"

class CommandContext;

class DynamicDescriptorHeap {
 public:
  DynamicDescriptorHeap(CommandContext& owning_context, D3D12_DESCRIPTOR_HEAP_TYPE type);

  void SetGraphicsDescriptorHandles(UINT root_index, UINT offset, UINT count, D3D12_CPU_DESCRIPTOR_HANDLE handles[]);
  void SetComputeDescriptorHandles(UINT root_index, UINT offset, UINT count, D3D12_CPU_DESCRIPTOR_HANDLE handles[]);

  void CommitGraphicsRootDescriptorTables(ID3D12GraphicsCommandList* command_list) {
    if (graphics_handle_cache_.stale_root_descriptor_table_bitmap != 0) {
      CopyAndBindStagedTables(graphics_handle_cache_, command_list,
          &ID3D12GraphicsCommandList::SetGraphicsRootDescriptorTable);
    }
  }

  void CommitComputeRootDescriptorTables(ID3D12GraphicsCommandList* command_list) {
    if (compute_handle_cache_.stale_root_descriptor_table_bitmap != 0) {
      CopyAndBindStagedTables(compute_handle_cache_, command_list,
          &ID3D12GraphicsCommandList::SetComputeRootDescriptorTable);
    }
  }

  void ParseGraphicsRootSignature(const RootSignature& root_signature) {
    graphics_handle_cache_.ParseRootSignature(heap_type_, root_signature);
  } 

  void ParseComputeRootSignature(const RootSignature& root_signature) {
    compute_handle_cache_.ParseRootSignature(heap_type_, root_signature);
  }

  void ClearupUsedHeaps(uint64_t fence_value);

 private:
  static const uint32_t kNumDescriptorsPerHeap = 1024;
  static vector<Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>> descriptor_heap_pool_[2];
  static queue<pair<uint64_t, ID3D12DescriptorHeap*>> retired_descriptor_heaps_[2];
  static queue<ID3D12DescriptorHeap*> avalible_descriptor_heaps_[2];
  
  static ID3D12DescriptorHeap* RequestDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type);
  static void DiscardDescriptorHeaps(D3D12_DESCRIPTOR_HEAP_TYPE type, uint64_t fence_value,  
      const vector<ID3D12DescriptorHeap*>& used_descriptor_heaps);
 private:

  struct DescriptorTableCache {
    DescriptorTableCache() 
      : assigned_handles_bitmap(0)
    {}
    D3D12_CPU_DESCRIPTOR_HANDLE* Start;
    uint32_t table_size;
    uint32_t assigned_handles_bitmap; //  bit mask of descriptor handles in table
  };

  bool HasSpace(uint32_t count) {
    return current_heap_ptr_ != nullptr && (current_offset_ + count < kNumDescriptorsPerHeap);
  }

  struct DescriptorHandleCache {
    const static UINT kMaxDescriptorCount = 256;
    const static UINT kMaxDescriptorTableCount = 16;

    uint32_t root_descriptor_tables_bitmap;
    uint32_t stale_root_descriptor_table_bitmap;  //  bit mask of using root index
    uint32_t max_cache_descriptors;
    

    DescriptorTableCache root_descriptor_tables[kMaxDescriptorCount];
    D3D12_CPU_DESCRIPTOR_HANDLE cache_handles[kMaxDescriptorTableCount];

    void StageTableHandles(UINT root_index, UINT offset, UINT count, D3D12_CPU_DESCRIPTOR_HANDLE handles[]);
    void ParseRootSignature(D3D12_DESCRIPTOR_HEAP_TYPE type, const RootSignature& root_signature);

    void CopyAndBindStaleTables(D3D12_DESCRIPTOR_HEAP_TYPE type, DescriptorHandle start_handle,
        uint32_t descriptor_size, ID3D12GraphicsCommandList* command_list,
        void (STDMETHODCALLTYPE ID3D12GraphicsCommandList::*set_function)(UINT, D3D12_GPU_DESCRIPTOR_HANDLE));

    uint32_t ComputeStagedSize();
    void UnbindValidTable();
    void ClearCache() {
      root_descriptor_tables_bitmap = 0;
      max_cache_descriptors = 0;
    } 
  };

 private:
  void CopyAndBindStagedTables(DescriptorHandleCache& handle_cache, 
    ID3D12GraphicsCommandList* command_list,
    void (STDMETHODCALLTYPE ID3D12GraphicsCommandList::* set_function)(UINT, D3D12_GPU_DESCRIPTOR_HANDLE));


  void RetireCurrentHeaps();
  void UnbindAllValid();
  DescriptorHandle Allocate(int count) {
    DescriptorHandle ret = first_handle_ + descriptor_size_* current_offset_;
    current_offset_ += count;
    return ret;
  }

  ID3D12DescriptorHeap* GetHeapPoint();
  void RetireUsedHeap(uint64_t fence_value );

 private:

  vector<ID3D12DescriptorHeap*> retired_heaps_;

  DescriptorHandleCache graphics_handle_cache_;
  DescriptorHandleCache compute_handle_cache_;

  CommandContext& owning_context_;
  const D3D12_DESCRIPTOR_HEAP_TYPE heap_type_;
  ID3D12DescriptorHeap* current_heap_ptr_;
  DescriptorHandle first_handle_;
  uint32_t current_offset_;
  UINT descriptor_size_;

};

#endif // !DYNAMICDESCRIPTORHEAP_H
