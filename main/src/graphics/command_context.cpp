#include "graphics/command_context.h"
#include "graphics/command_list_manager.h"
#include "game/game_core.h"


#include "memory_utility.h"

ContextManager::ContextManager()
{
}

//
//ContextManager::~ContextManager()
//{
//
//}

CommandContext* ContextManager::Allocate(D3D12_COMMAND_LIST_TYPE type) {

  auto& contexts = avalible_contexts_[type];
  CommandContext* ret = nullptr;

  if (contexts.empty()) {
    //  std::cout << "contexts.empty()" << std::endl;
    OutputDebugString(L"contexts.empty()\n");
    ret = new CommandContext(type);
    ret->Initialize();
    context_pool_[type].emplace_back(ret);
  }
  else {
    //  std::cout << "contexts.pop()" << std::endl;
    ret = contexts.front();
    contexts.pop();
    ret->Reset();
  }

  assert(ret != nullptr && "The expect context is null");

  return ret;
}

void ContextManager::Free(CommandContext* used_context) {
  assert(used_context != nullptr && "ContextManager::Free exception:used context is nullptr");
  avalible_contexts_[used_context->type_].push(used_context);
}

CommandContext::CommandContext(D3D12_COMMAND_LIST_TYPE type)
    : type_(type),
    command_allocator_(nullptr),
    command_list_(nullptr),
    cpu_linear_allocator_(kCpuWritable),
    gpu_linear_allocator_(kGpuExclusive),
    graphics_signature_(nullptr),
    compute_signature_(nullptr),
    graphics_pso_(nullptr) ,
    dynamic_view_descriptor_heap_(*this, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV),
    dynamic_sampler_descriptor_heap_(*this, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER)
    //    dynamic_view_descriptor_heap_(*this)
    {
  ZeroMemory(bind_descriptor_heaps_, sizeof(bind_descriptor_heaps_));
  barrier_to_flush = 0;
}

CommandContext::~CommandContext() {
  command_list_->Release();
}

void CommandContext::Initialize() {
  CommandListManager::Instance().CreateNewCommandList(type_, &command_list_, &command_allocator_);
  //  Graphics::Core.CommandManager().CreateNewCommandList(type_, &command_list_, &command_allocator_);

  assert(command_list_ != nullptr && "command list is nullptr");
  assert(command_allocator_ != nullptr && "command allocator is nullptr");

  ThrowIfFailed(command_list_->Close());
  command_list_->Reset(command_allocator_, nullptr);
}

void CommandContext::Reset() {
  assert(command_list_ != nullptr && command_allocator_ == nullptr);
  command_allocator_ = CommandListManager::Instance().GetQueue(type_).RequestAllocator();
  //  command_allocator_->Reset();
  command_list_->Reset(command_allocator_, nullptr);

  graphics_pso_ = nullptr;
  graphics_signature_ = nullptr;

  compute_pso_ = nullptr;
  compute_signature_ = nullptr;

  barrier_to_flush = 0;
  BindDescriptorHeaps();
}

CommandContext& CommandContext::Begin(const std::wstring& name) {
  CommandContext* context = ContextManager::Instance().Allocate(D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT);
  context->command_list_->SetName(name.c_str());
  return *context;
}

void CommandContext::InitializeBuffer(GpuResource& resource, const void* buffer_data, size_t byte_size, size_t offset) {
  CommandContext& init_context = CommandContext::Begin(L"Init Context");

  DynAlloc memory = init_context.ReserveUploadMemory(byte_size);
  MemoryUtility::SIMDMemCopy(memory.Data, buffer_data, Math::DivideByMultiple(byte_size, 16));

  init_context.TransitionResource(resource, D3D12_RESOURCE_STATE_COPY_DEST, true);
  init_context.command_list_->CopyBufferRegion(resource.GetResource(), offset, memory.Buffer.GetResource(), 0, byte_size);
  init_context.TransitionResource(resource, D3D12_RESOURCE_STATE_GENERIC_READ, true);

  init_context.Finish(true);
}


uint64_t CommandContext::Flush(bool wait_for_signal) {  //  CommandQueue* command_queue, 
  //assert(command_queue != nullptr);
  FlushResourceBarriers();

  assert(command_allocator_ != nullptr);

  auto& command_queue = CommandListManager::Instance().GetQueue(type_);
  uint64_t fence_value = command_queue.ExecuteCommandList(command_list_);
  
  if (wait_for_signal) {
    command_queue.WaitForSignal(fence_value);
  }
  
  command_list_->Reset(command_allocator_, nullptr);

  if (graphics_signature_ != nullptr) {
    command_list_->SetGraphicsRootSignature(graphics_signature_);
    command_list_->SetPipelineState(graphics_pso_);
  }
  if (compute_signature_ != nullptr) {
    command_list_->SetComputeRootSignature(compute_signature_);
    command_list_->SetPipelineState(compute_pso_);
  }

  BindDescriptorHeaps();
  
  return fence_value;
}

uint64_t CommandContext::Finish(bool wait_for_signal) {
  FlushResourceBarriers();

  CommandQueue& queue = CommandListManager::Instance().GetQueue(type_);
  
  uint64_t fence_value = queue.ExecuteCommandList(command_list_);
  queue.DiscardAllocator(fence_value, command_allocator_);
  command_allocator_ = nullptr;

  dynamic_view_descriptor_heap_.ClearupUsedHeaps(fence_value);
  dynamic_sampler_descriptor_heap_.ClearupUsedHeaps(fence_value);

  cpu_linear_allocator_.CleanupUsedPages(fence_value);
  gpu_linear_allocator_.CleanupUsedPages(fence_value);


  if (wait_for_signal) {
    queue.WaitForSignal(fence_value);
  }

  ContextManager::Instance().Free(this);
  return fence_value;
}

void CommandContext::InsertUavBarrier(GpuResource& resource, bool flush_immediate) {
  D3D12_RESOURCE_BARRIER &barrier = barrier_buffer[barrier_to_flush++];
  barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
  barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
  barrier.UAV.pResource = resource.GetResource();

  if (flush_immediate) {
    FlushResourceBarriers();
  }
}

void CommandContext::TransitionResource(GpuResource& resource, D3D12_RESOURCE_STATES state_new, bool flush_immediate) {
  D3D12_RESOURCE_STATES state_before = resource.current_state_;

  if (type_ == D3D12_COMMAND_LIST_TYPE_COMPUTE)
  {
    assert((state_before & VALID_COMPUTE_QUEUE_RESOURCE_STATES) == state_before);
    assert((state_new & VALID_COMPUTE_QUEUE_RESOURCE_STATES) == state_new);
  }


  if (state_before != state_new) {
    D3D12_RESOURCE_BARRIER &barrier = barrier_buffer[barrier_to_flush++];
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

    barrier.Transition.pResource = resource.resource_.Get();
    barrier.Transition.StateBefore = state_before;
    barrier.Transition.StateAfter = state_new;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    resource.current_state_ = state_new;
  }
  else if (state_new == D3D12_RESOURCE_STATE_UNORDERED_ACCESS) {
    InsertUavBarrier(resource, flush_immediate);
  }


  if (flush_immediate || barrier_to_flush >= kBarrierBufferCount) {
    FlushResourceBarriers();
  }
}

void CommandContext::BindDescriptorHeaps() {
  UINT not_null_heap = 0;
  ID3D12DescriptorHeap* to_bind_heaps[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];
  for (int i=0; i<D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i) {
    ID3D12DescriptorHeap* heap_iter = bind_descriptor_heaps_[i];
    if (heap_iter != nullptr) {
      to_bind_heaps[not_null_heap] = heap_iter;
      not_null_heap++;
    }
  }
  
  if (not_null_heap > 0) {
    command_list_->SetDescriptorHeaps(not_null_heap, to_bind_heaps);
  }
}

GraphicsContext& GraphicsContext::Begin(const std::wstring& name) {
  CommandContext& context = CommandContext::Begin(name);
  return context.GetGraphicsContext();
}

void GraphicsContext::ClearColor(ColorBuffer& target) {
  command_list_->ClearRenderTargetView(target.Rtv(), target.GetColor().Ptr(), 0, nullptr);
}

void GraphicsContext::ClearDepth(DepthStencilBuffer& target) {
  command_list_->ClearDepthStencilView(target.DSV(), D3D12_CLEAR_FLAG_DEPTH ,
    target.ClearDepth(), target.ClearStencil(), 0, nullptr);
}

void GraphicsContext::ClearStencil(DepthStencilBuffer& target) {
  command_list_->ClearDepthStencilView(target.DSV(), D3D12_CLEAR_FLAG_STENCIL,
    target.ClearDepth(), target.ClearStencil(), 0, nullptr);
}

void GraphicsContext::ClearDepthStencil(DepthStencilBuffer& target) {
  command_list_->ClearDepthStencilView(target.DSV(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 
      target.ClearDepth(), target.ClearStencil(), 0, nullptr);
}

void GraphicsContext::SetRenderTarget(const D3D12_CPU_DESCRIPTOR_HANDLE rtvs[]) {
  command_list_->OMSetRenderTargets(1, rtvs, true, nullptr);
}

void GraphicsContext::SetRenderTargets(UINT rtv_count, const D3D12_CPU_DESCRIPTOR_HANDLE rtvs[], D3D12_CPU_DESCRIPTOR_HANDLE dsv) {
  command_list_->OMSetRenderTargets(rtv_count, rtvs, true, &dsv);
}

ComputeContext& ComputeContext::Begin(const std::wstring& name, bool is_async) {
  ComputeContext& context = ContextManager::Instance().Allocate(
      is_async ? D3D12_COMMAND_LIST_TYPE_COMPUTE : D3D12_COMMAND_LIST_TYPE_DIRECT)->GetComputeContext();
  context.SetName(name);
  return context;
}







