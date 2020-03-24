#pragma once

#ifndef COMMANDALLOCATORPOOL_H
#define COMMANDALLOCATORPOOL_H

#include "utility.h"

class CommandAllocatorPool
{
 public:
  CommandAllocatorPool(D3D12_COMMAND_LIST_TYPE type);
  ~CommandAllocatorPool();

  void Create(ID3D12Device* device);

  ID3D12CommandAllocator* RequestAllocator(uint64_t complete_fence_value);
  void DiscardAllocator(uint64_t complete_fence_value, ID3D12CommandAllocator* allocator);
  

 private:
  std::queue<std::pair<uint64_t, ID3D12CommandAllocator*>> ready_allocators_;
  std::vector<ID3D12CommandAllocator*> allocator_pool_;
  ID3D12Device* device_;
  D3D12_COMMAND_LIST_TYPE type_;
};


#endif // !COMMANDALLOCATORPOOL_H




