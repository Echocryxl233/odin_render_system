#include "color_buffer.h"
#include "descriptor_allocator.h"
#include "graphics_core.h"

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

  rtv_handle_ = DescriptorAllocatorManager::Instance().Allocate(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
  device->CreateRenderTargetView(resource_.Get(), nullptr, rtv_handle_ );
  resource_->SetName(name.c_str());
}

void ColorBuffer::Create(uint32_t width, uint32_t height, DXGI_FORMAT format, D3D12_RESOURCE_FLAGS flags) {
  auto desciptor = DescribeTexture2D(width, height, format, 1, 1, flags);
  auto* device = Graphics::Core.Device();
    
  D3D12_CLEAR_VALUE clear_value = {};
  clear_value.Format = format;
  clear_value.DepthStencil.Depth = 0;
  clear_value.DepthStencil.Stencil = 0;
  memcpy(clear_value.Color, color_.Ptr(), sizeof(clear_value.Color));
  CreateTextureResource(device, L"", desciptor, clear_value);

  D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
  rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
  rtvDesc.Format = format;
  rtvDesc.Texture2D.MipSlice = 0;
  rtvDesc.Texture2D.PlaneSlice = 0;

  rtv_handle_ = DescriptorAllocatorManager::Instance().Allocate(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
  device->CreateRenderTargetView(resource_.Get(), &rtvDesc, rtv_handle_);

  D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
  srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
  srvDesc.Format = format;
  srvDesc.Texture2D.MostDetailedMip = 0;
  srvDesc.Texture2D.MipLevels = 1;

  srv_handle_ = DescriptorAllocatorManager::Instance().Allocate(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
  device->CreateShaderResourceView(resource_.Get(), &srvDesc, srv_handle_);

  DebugUtility::Log(L"ColorBuffer::Create");
}
