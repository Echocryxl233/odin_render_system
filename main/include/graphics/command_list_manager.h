#pragma once

#ifndef COMMANDLISTMANAGER_H
#define COMMANDLISTMANAGER_H

#include "graphics/command_context.h"
#include "graphics/command_queue.h"
#include "utility.h"

class CommandListManager : public Singleton<CommandListManager> {
friend class Singleton<CommandListManager>;
 public:
  
  //static CommandListManager& Instance();
  void Initialize(ID3D12Device* device);

  CommandQueue& GetQueue(D3D12_COMMAND_LIST_TYPE type) {
    switch (type)
    {
    case D3D12_COMMAND_LIST_TYPE_BUNDLE:
      break;
    case D3D12_COMMAND_LIST_TYPE_COMPUTE:
      break;
    case D3D12_COMMAND_LIST_TYPE_COPY:
      break;
    case D3D12_COMMAND_LIST_TYPE_VIDEO_DECODE:
      break;
    case D3D12_COMMAND_LIST_TYPE_VIDEO_PROCESS:
      break;
    default:
      return graphics_queue_;
      break;
    }
    return graphics_queue_;
  }

  ID3D12CommandQueue* GetCommandQueue()
  {
    return graphics_queue_.GetCommandQueue();
  }

  bool IsFenceComplete(uint64_t fence_value) {
    return GetQueue(D3D12_COMMAND_LIST_TYPE(fence_value >> 56)).IsFenceComplete(fence_value);
  }

  void CreateNewCommandList(D3D12_COMMAND_LIST_TYPE type, ID3D12GraphicsCommandList** list, ID3D12CommandAllocator** allocator);

 private:
  CommandListManager() ;

 private:
  
  ID3D12Device* device_;

  CommandListManager(const CommandListManager&) = delete;
  CommandListManager operator=(const CommandListManager&) = delete;

  CommandQueue graphics_queue_;

};


#endif // !COMMANDLISTMANAGER_H

