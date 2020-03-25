#include "graphics/command_list_manager.h"

CommandListManager::CommandListManager() 
    : graphics_queue_(D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT)
    {
  
}

void CommandListManager::Initialize(ID3D12Device* device) {
  device_ = device;

  graphics_queue_.Create(device);
  graphics_queue_.SetName(L"CommandListManage::graphics queue");
}

void CommandListManager::CreateNewCommandList(D3D12_COMMAND_LIST_TYPE type, ID3D12GraphicsCommandList** list, ID3D12CommandAllocator** allocator) {
  switch (type)
  {
    case D3D12_COMMAND_LIST_TYPE_DIRECT:
      *allocator = graphics_queue_.RequestAllocator();
      break;
  }

  ThrowIfFailed(device_->CreateCommandList(1, type, *allocator, nullptr, IID_PPV_ARGS(list)));
  (*list)->SetName(L"graphics list");
  (*allocator)->SetName(L"graphics allocator");
}