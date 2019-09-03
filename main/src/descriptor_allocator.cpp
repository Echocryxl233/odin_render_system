#include <iostream>

#include "descriptor_allocator.h"
#include "graphics_core.h"

//  std::vector<Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>> DescriptorAllocator::allocaltor_pool_;

DescriptorAllocatorManager::DescriptorAllocator::DescriptorAllocator(D3D12_DESCRIPTOR_HEAP_TYPE type)
    : type_(type), 
      current_heap_(nullptr)
{
}


DescriptorAllocatorManager::DescriptorAllocator::~DescriptorAllocator()
{
}

ID3D12DescriptorHeap* DescriptorAllocatorManager::DescriptorAllocator::RequestNewHeap(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type) {
  D3D12_DESCRIPTOR_HEAP_DESC heap_desc;
  heap_desc.Type = type;
  heap_desc.NumDescriptors = kDescriptorCount;
  heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAGS::D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
  heap_desc.NodeMask = 0;
    
  Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> heap;
  ThrowIfFailed(device->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(heap.GetAddressOf())));
  allocaltor_pool_.emplace_back(heap);

  DebugUtility::Log(L"DescriptorAllocator::RequestNewHeap, type = %0, pool count %1", 
      to_wstring(type), to_wstring(allocaltor_pool_.size()));
  return heap.Get();
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorAllocatorManager::DescriptorAllocator::Allocate(ID3D12Device* device, uint32_t descriptor_count) {
  if (current_heap_ == nullptr || remainning_descriptor_count_ < descriptor_count) {
    current_heap_ = RequestNewHeap(device, type_);
    current_handle_ = current_heap_->GetCPUDescriptorHandleForHeapStart();
    remainning_descriptor_count_ = kDescriptorCount;
  }

  assert(current_heap_ != nullptr && "allocate the null descriptor heap");

  uint32_t desciptor_size = device->GetDescriptorHandleIncrementSize(type_);

  D3D12_CPU_DESCRIPTOR_HANDLE ret = current_handle_;
  remainning_descriptor_count_ -= descriptor_count;
  current_handle_.ptr += (descriptor_count * desciptor_size);
  return ret;
}

//ID3D12DescriptorHeap* DescriptorPool::RequestNewHeap(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type) {
//  D3D12_DESCRIPTOR_HEAP_DESC heap_desc;
//  heap_desc.Type = type;
//  heap_desc.NumDescriptors = DescriptorAllocator::kDescriptorCount;
//  heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAGS::D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
//  heap_desc.NodeMask = 0;
//
//  Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> heap;
//  ThrowIfFailed(device->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(heap.GetAddressOf())));
//  allocaltor_pool_.emplace_back(heap);
//
//  return heap.Get();
//}

DescriptorAllocatorManager::DescriptorAllocatorManager() {
  int count = (int)D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES;
  for (int i=0; i<count; ++i) {
    D3D12_DESCRIPTOR_HEAP_TYPE type = (D3D12_DESCRIPTOR_HEAP_TYPE)i;
    allocators_[i] = std::make_shared<DescriptorAllocator>(type);
  }
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorAllocatorManager::Allocate(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t descriptor_count) {
  ID3D12Device* device = Graphics::Core.Device();

  return allocators_[type]->Allocate(device, descriptor_count);
  
}

