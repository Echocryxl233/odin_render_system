#pragma once

#ifndef LINEARALLOCATOR_H
#define LINEARALLOCATOR_H

#include "gpu_resource.h"

using namespace std;

#define DEFAULT_ALIGN 256

struct DynAlloc
{
  DynAlloc(GpuResource& resource, size_t offset, size_t size)
    : Buffer(resource), Offset(offset), Size(size) {}

  GpuResource& Buffer;    // The D3D buffer associated with this memory.
  size_t Offset;            // Offset from start of buffer resource
  size_t Size;            // Reserved size of this allocation
  void* Data;            // The CPU-writeable address
  D3D12_GPU_VIRTUAL_ADDRESS GpuAddress;    // The GPU-visible address
};

class LinearAllocationPage : public GpuResource {
 public:
  LinearAllocationPage(ID3D12Resource* resource, D3D12_RESOURCE_STATES state) {
    resource_.Attach(resource);
    current_state_ = state;
    GpuAddress = resource->GetGPUVirtualAddress();
    resource_->Map(0, nullptr, &CpuAddress);
  }

  ~LinearAllocationPage() {
    Unmap();
  }
  void Map() {
    if (nullptr == CpuAddress) {
      resource_->Map(0, nullptr, &CpuAddress);
    }
  }
  
  void Unmap() {
    if (CpuAddress != nullptr) {
      CpuAddress = nullptr;
      resource_->Unmap(0, nullptr);
    }
  }

  void* CpuAddress;
  D3D12_GPU_VIRTUAL_ADDRESS GpuAddress;
};

enum LinearAllocatorType
{
  kInvalidAllocator = -1,

  kGpuExclusive = 0,        // DEFAULT   GPU-writeable (via UAV)
  kCpuWritable = 1,        // UPLOAD CPU-writeable (but write combined)

  
  kNumAllocatorTypes
};

enum PageSize
{
  kGpuAllocatorPageSize = 0x10000,    // 64K
  kCpuAllocatorPageSize = 0x200000    // 2MB
};

class LinearAllocatorPageManager {
 public:
  LinearAllocatorPageManager();
  LinearAllocationPage* RequestPage();
  LinearAllocationPage* CreateNewPage(size_t page_size = 0);
  void DiscardPages(uint64_t fence_value, const vector<LinearAllocationPage*> pages);
  void FreeLargePages(uint64_t fence_value, const vector<LinearAllocationPage*> pages);
  void Destroy(void) { page_pool_.clear(); }

 private:

  static LinearAllocatorType auto_type_;
  
  LinearAllocatorType allocator_type_;
  queue<pair<uint64_t, LinearAllocationPage*>> retired_pages_;
  queue<pair<uint64_t, LinearAllocationPage*>> deleted_pages_;
  vector<unique_ptr<LinearAllocationPage>> page_pool_;
  queue<LinearAllocationPage*> avalible_pages_;

};

class LinearAllocator {
 public:
  LinearAllocator(LinearAllocatorType type);
  DynAlloc Allocate(size_t page_size, size_t alignment = DEFAULT_ALIGN);
  void CleanupUsedPages(uint64_t FenceID);

  static void DestroyAll(void)
  {
    page_managers_[0].Destroy();
    page_managers_[1].Destroy();
  }

 private:
  DynAlloc AllocateLargePage(size_t size_in_bytes);

  static LinearAllocatorPageManager page_managers_[2];

  LinearAllocatorType allocation_type_;
  size_t page_size_;
  size_t page_offset_;
  LinearAllocationPage* current_page_;
  vector<LinearAllocationPage*> retired_pages_;
  vector<LinearAllocationPage*> large_page_list_;
  
};

#endif