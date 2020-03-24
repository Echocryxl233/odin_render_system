#include "graphics/command_allocator_pool.h"
#include <iostream>


CommandAllocatorPool::CommandAllocatorPool(D3D12_COMMAND_LIST_TYPE type)
    : type_(type)
{
}

void CommandAllocatorPool::Create(ID3D12Device* device) {
  device_ = device;
}

CommandAllocatorPool::~CommandAllocatorPool()
{
  for(int i=0; i<allocator_pool_.size(); ++i) {
    allocator_pool_[i]->Release();
  }
  allocator_pool_.clear();
}

ID3D12CommandAllocator* CommandAllocatorPool::RequestAllocator(uint64_t complete_fence_value) {
  ID3D12CommandAllocator* allocator = nullptr;
  
  if (!ready_allocators_.empty()) {
    auto& allocator_pair = ready_allocators_.front();
    if (allocator_pair.first <= complete_fence_value) {
      //  std::cout << "CommandAllocatorPool::RequestAllocator pop()" << std::endl;
      allocator = allocator_pair.second;
      ready_allocators_.pop();
      ThrowIfFailed(allocator->Reset());
    }
  }

  if (allocator == nullptr) {
    ThrowIfFailed(device_->CreateCommandAllocator(type_, IID_PPV_ARGS(&allocator)));
    
    

    allocator_pool_.push_back(allocator);
    DebugLog::Print(L"CommandAllocatorPool::RequestAllocator size = %0", std::to_wstring(allocator_pool_.size()));
  }

  return allocator;
   
}

void CommandAllocatorPool::DiscardAllocator(uint64_t fence_value, ID3D12CommandAllocator* allocator) {
  auto allocator_pair = std::make_pair(fence_value, allocator);
  ready_allocators_.push(allocator_pair);
}
