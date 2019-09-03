#include "color_buffer.h"
#include "descriptor_allocator.h"


ColorBuffer::ColorBuffer(Color color)
    : color_(color)
{
  srv_handle_.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
  rtv_handle_.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
}


ColorBuffer::~ColorBuffer()
{
}

void ColorBuffer::CreateFromSwapChain(std::wstring& name, ID3D12Device* device, ID3D12Resource* resource) {
  assert(device != nullptr && "ColorBuffer::CreateFromSwapChain exception: device is nullptr");
  assert(resource != nullptr && "ColorBuffer::CreateFromSwapChain exception: resource is nullptr");

  AssoiateWithResource(resource, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PRESENT);
  //  descriptor_handle_ = descriptor_handle;
  
  // rtv_handle_ = descriptor_handle;
  rtv_handle_ = DescriptorAllocatorManager::Instance().Allocate(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
  device->CreateRenderTargetView(resource_.Get(), nullptr, rtv_handle_ );
  resource_->SetName(name.c_str());
}
