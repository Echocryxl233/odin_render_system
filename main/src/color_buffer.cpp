#include "color_buffer.h"
#include "descriptor_allocator.h"
#include "graphics_core.h"

ColorBuffer::ColorBuffer(Color color)
    : color_(color), mips_count_(0), fragment_count_(1)
{
  srv_handle_.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
  rtv_handle_.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
  memset(uav_handle_, 0xFF, sizeof(uav_handle_));
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

void ColorBuffer::Create(wstring name, uint32_t width, uint32_t height, uint32_t mips_count, DXGI_FORMAT format) {
  D3D12_RESOURCE_FLAGS combined_flags = CombineFlags();

  mips_count = (mips_count == 0) ? ComputeNumMips(width, height) : mips_count;

  auto desciptor = DescribeTexture2D(width, height, format, 1, mips_count, combined_flags);
    
  D3D12_CLEAR_VALUE clear_value = {};
  clear_value.Format = format;
  clear_value.Color[0] = color_.R();
  clear_value.Color[1] = color_.G();
  clear_value.Color[2] = color_.B();
  clear_value.Color[3] = color_.A();

  //  memcpy(clear_value.Color, color_.Ptr(), sizeof(clear_value.Color));
  CreateTextureResource(name, desciptor, clear_value);
  CreateDeriveView(format, mips_count);
  DebugUtility::Log(L"ColorBuffer::Create");
}

void ColorBuffer::CreateDeriveView(DXGI_FORMAT format, uint32_t mips_count) {
  auto* device = Graphics::Core.Device();

  mips_count_ = mips_count - 1;

  D3D12_RENDER_TARGET_VIEW_DESC rtv_desc = {};
  rtv_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
  rtv_desc.Format = format;
  rtv_desc.Texture2D.MipSlice = 0;
  rtv_desc.Texture2D.PlaneSlice = 0;

  rtv_handle_ = DescriptorAllocatorManager::Instance().Allocate(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
  device->CreateRenderTargetView(resource_.Get(), &rtv_desc, rtv_handle_);

  D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
  srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
  srv_desc.Format = format;
  srv_desc.Texture2D.MostDetailedMip = 0;
  srv_desc.Texture2D.MipLevels = 1;

  srv_handle_ = DescriptorAllocatorManager::Instance().Allocate(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
  device->CreateShaderResourceView(resource_.Get(), &srv_desc, srv_handle_);

  D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};

  uav_desc.Format = GetUAVFormat(format);
  uav_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
  uav_desc.Texture2D.MipSlice = 0;

  if (fragment_count_ > 1)
    return ;

  for (int i=0; i<mips_count; ++i) {
    uav_handle_[i] = DescriptorAllocatorManager::Instance().Allocate(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    device->CreateUnorderedAccessView(resource_.Get(), nullptr, &uav_desc, uav_handle_[i]);
  }



}
