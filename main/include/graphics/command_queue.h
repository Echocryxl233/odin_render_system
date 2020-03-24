#pragma once

#ifndef COMMANDQUEUE_H
#define COMMANDQUEUE_H

#include <d3d12.h>
#include <stdint.h>

#include "utility.h"

#include "graphics/command_allocator_pool.h"

class CommandQueue {
 public:
  CommandQueue(D3D12_COMMAND_LIST_TYPE type);
  ~CommandQueue();

  void Create(ID3D12Device* device);

  void SetName(std::wstring name) {
    name_ = name;
    command_queue_->SetName(name.c_str());
  }

  std::wstring GetName() {
    
    return name_;
  } 

  void Shutdown();

  uint64_t IncreaseFence();
  bool IsFenceComplete(uint64_t fence_value);
  void WaitForSignal(uint64_t fence_value);

  ID3D12CommandQueue* GetCommandQueue() { return command_queue_;}  
  uint64_t ExecuteCommandList(ID3D12CommandList* list);

  ID3D12CommandAllocator* RequestAllocator();
  void DiscardAllocator(uint64_t fence_value, ID3D12CommandAllocator* allocator);

  uint64_t GetNextFenceValue() { return next_fence_value_; }
  
  //  ID3D12Fence* Fence() { return fence_; }

  void FlushGpu();

 private:
  
  ID3D12CommandQueue* command_queue_;
  uint64_t next_fence_value_;
  uint64_t last_complete_fence_value_;
  ID3D12Fence* fence_;
  HANDLE fence_event_handle_;
  const D3D12_COMMAND_LIST_TYPE type_;

  CommandAllocatorPool command_allocator_pool_;

  std::wstring name_;

};


#endif // !COMMANDQUEUE_H

