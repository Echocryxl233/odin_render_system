#include "graphics/command_queue.h"

CommandQueue::CommandQueue(D3D12_COMMAND_LIST_TYPE type) 
    : type_(type),
      fence_(nullptr),
      next_fence_value_((uint64_t)type << 56 | 1),
      last_complete_fence_value_((uint64_t)type << 56),
      command_queue_(nullptr),
      fence_event_handle_(nullptr),
      command_allocator_pool_(type)
{
  
}

CommandQueue::~CommandQueue() {
  Shutdown();
}

void CommandQueue::Create(ID3D12Device* device) {
  assert(device != nullptr && "CommandQueue::Create exception, device is nullptr");

  D3D12_COMMAND_QUEUE_DESC queue_desc = {};
  queue_desc.Type = type_;
  queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAGS::D3D12_COMMAND_QUEUE_FLAG_NONE;
  ThrowIfFailed(device->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&command_queue_)));

  ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAGS::D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence_)));

  fence_->Signal(last_complete_fence_value_);

  fence_event_handle_ = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);
  assert(fence_event_handle_ != nullptr && "Create fence event handle failed");

  command_allocator_pool_.Create(device);
}

void CommandQueue::Shutdown() {
  if (command_queue_ == nullptr)
    return;

  CloseHandle(fence_event_handle_);

  fence_->Release();
  fence_ = nullptr;

  command_queue_->Release();
  command_queue_ = nullptr;
}

uint64_t CommandQueue::ExecuteCommandList(ID3D12CommandList* list) {

  auto hr = ((ID3D12GraphicsCommandList*)list)->Close();
  ThrowIfFailed(hr);
  command_queue_->ExecuteCommandLists(1, &list);

  command_queue_->Signal(fence_, next_fence_value_);
  uint64_t fence_value = next_fence_value_;
  ++next_fence_value_;
  return fence_value;
}

ID3D12CommandAllocator* CommandQueue::RequestAllocator() {
  assert(fence_ != nullptr && "Fence is null, forget to create command queue...");
  uint64_t complete_fence_value = fence_->GetCompletedValue();
  //  uint64_t complete_fence_value = 0ll;
  return command_allocator_pool_.RequestAllocator(complete_fence_value);
}

void CommandQueue::DiscardAllocator(uint64_t fence_value, ID3D12CommandAllocator* allocator) {
  command_allocator_pool_.DiscardAllocator(fence_value, allocator);
}

uint64_t CommandQueue::IncreaseFence() {
  ThrowIfFailed(command_queue_->Signal(fence_, next_fence_value_));
  ++next_fence_value_;
  return next_fence_value_;
}

bool CommandQueue::IsFenceComplete(uint64_t fence_value) {
  if (last_complete_fence_value_ < fence_value) {
    auto complete_value =  fence_->GetCompletedValue();
    last_complete_fence_value_ = (std::max)(last_complete_fence_value_, complete_value);
  }

  return last_complete_fence_value_ >= fence_value;
}

void CommandQueue::WaitForSignal(uint64_t fence_value) {
  if (IsFenceComplete(fence_value))
    return;
  
  //  HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);

  // Fire event when GPU hits current fence.  
  ThrowIfFailed(fence_->SetEventOnCompletion(fence_value, fence_event_handle_));

  // Wait until the GPU hits current fence event is fired.
  WaitForSingleObject(fence_event_handle_, INFINITE);
  last_complete_fence_value_ = fence_value;
}

void CommandQueue::FlushGpu() {
  WaitForSignal(next_fence_value_);
}