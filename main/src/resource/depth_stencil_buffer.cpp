#include "resource/depth_stencil_buffer.h"
#include "graphics/command_context.h"
#include "graphics/graphics_core.h"

DepthStencilBuffer::DepthStencilBuffer(float depth, uint8_t stencil) 
    : depth_(depth),
      stencil_(stencil)
    {
  dsv_[0].ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
  dsv_[1].ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
  dsv_[2].ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
  dsv_[3].ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
  depth_srv_.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
  stencil_srv_.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
}

void DepthStencilBuffer::Create(const wstring& name, uint32_t width, uint32_t height, DXGI_FORMAT format) {
  ID3D12Device* device = Graphics::Core.Device();

  auto resource_desc = DescribeTexture2D(width, height, format, 1, 1, 
      D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

  D3D12_CLEAR_VALUE clear_value = {};
  clear_value.Format = format;
  clear_value.DepthStencil.Depth = depth_;
  clear_value.DepthStencil.Stencil = stencil_;

  CreateTextureResource(name, resource_desc, clear_value);
  CreateDeriveView(device, format);

  GraphicsContext& context = GraphicsContext::Begin(L"Depth Command");
  context.TransitionResource(*this, D3D12_RESOURCE_STATE_DEPTH_WRITE, true);
  context.Finish();
}

void DepthStencilBuffer::CreateDeriveView(ID3D12Device* device, DXGI_FORMAT format) {
  ID3D12Resource* resource = resource_.Get();
  
  D3D12_DEPTH_STENCIL_VIEW_DESC dsv_desc;
  dsv_desc.Flags = D3D12_DSV_FLAG_NONE;
  dsv_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
  dsv_desc.Format = format_;
  dsv_desc.Texture2D.MipSlice = 0;

  auto desc_heap_type = D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
  if (dsv_[0].ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN) {
    dsv_[0] = DescriptorAllocatorManager::Instance().Allocate(desc_heap_type);
    dsv_[1] = DescriptorAllocatorManager::Instance().Allocate(desc_heap_type);
  }
  
  dsv_desc.Flags = D3D12_DSV_FLAG_NONE;
  device->CreateDepthStencilView(resource, &dsv_desc, dsv_[0]);

  dsv_desc.Flags = D3D12_DSV_FLAG_READ_ONLY_DEPTH;
  device->CreateDepthStencilView(resource, &dsv_desc, dsv_[1]);

  DXGI_FORMAT stencil_read_format = GetStencilFormat(format);
  if (stencil_read_format != DXGI_FORMAT::DXGI_FORMAT_UNKNOWN) {
    if (dsv_[2].ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN) {
      dsv_[2] = DescriptorAllocatorManager::Instance().Allocate(desc_heap_type);
      dsv_[3] = DescriptorAllocatorManager::Instance().Allocate(desc_heap_type);
    }

    dsv_desc.Flags = D3D12_DSV_FLAG_READ_ONLY_STENCIL;
    device->CreateDepthStencilView(resource, &dsv_desc, dsv_[2]);

    dsv_desc.Flags = D3D12_DSV_FLAG_READ_ONLY_DEPTH | D3D12_DSV_FLAG_READ_ONLY_STENCIL;
    device->CreateDepthStencilView(resource, &dsv_desc, dsv_[3]);
  }
  else {
    dsv_[2] = dsv_[0];
    dsv_[3] = dsv_[1];
  }

  if (depth_srv_.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN) {
    depth_srv_ = DescriptorAllocatorManager::Instance().Allocate(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
  }

  D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};

  srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  srv_desc.Format = GetDepthFormat(format);
  srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
  srv_desc.Texture2D.MostDetailedMip = 0;
  srv_desc.Texture2D.MipLevels = 1;
  
  device->CreateShaderResourceView(resource, &srv_desc, depth_srv_);
   
  if (stencil_read_format != DXGI_FORMAT_UNKNOWN) {
    if (stencil_srv_.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN) {
      stencil_srv_ = DescriptorAllocatorManager::Instance().Allocate(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    }
    srv_desc.Format = stencil_read_format;
    device->CreateShaderResourceView(resource, &srv_desc, stencil_srv_);
  }
  
}