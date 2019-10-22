#include "pixel_buffer.h"
#include "graphics_core.h"


PixelBuffer::PixelBuffer()
{
}


PixelBuffer::~PixelBuffer()
{
}

void PixelBuffer::AssoiateWithResource(ID3D12Resource* resource, D3D12_RESOURCE_STATES state_usage) {
  assert(resource != nullptr);

  resource_.Attach(resource);
  current_state_ = state_usage;

  auto resource_desc = resource_->GetDesc();
  
  width_ = (uint32_t)resource_desc.Width;
  height_ = resource_desc.Height;
  format_ = resource_desc.Format;
}

D3D12_RESOURCE_DESC PixelBuffer::DescribeTexture2D(uint32_t width, uint32_t height, DXGI_FORMAT format,
    uint32_t array_size, uint32_t mip_level, D3D12_RESOURCE_FLAGS flags) {
  width_ = width;
  height_ = height;
  format_ = format;

  D3D12_RESOURCE_DESC resource_desc = {};
  resource_desc.Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE2D;
  resource_desc.Alignment = 0;
  resource_desc.Width = width;
  resource_desc.Height = height;
  resource_desc.DepthOrArraySize = (UINT16)array_size;
  resource_desc.MipLevels = (UINT16)mip_level;
  resource_desc.Format = GetBaseFormat(format);
  resource_desc.SampleDesc.Quality = 0;
  resource_desc.SampleDesc.Count = 1;
  resource_desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
  resource_desc.Flags = flags;

  return resource_desc;
}

void PixelBuffer::CreateTextureResource(const std::wstring& name, D3D12_RESOURCE_DESC& descriptor, D3D12_CLEAR_VALUE& clear_value) {
  Destroy();
  
  CD3DX12_HEAP_PROPERTIES properties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

  current_state_ = D3D12_RESOURCE_STATE_COMMON;
  ID3D12Device* device = Graphics::Core.Device();
  ASSERT_SUCCESSED(device->CreateCommittedResource(&properties, D3D12_HEAP_FLAG_NONE, &descriptor,
    current_state_, &clear_value, IID_PPV_ARGS(resource_.GetAddressOf())));
  resource_->SetName(name.c_str());
}

DXGI_FORMAT PixelBuffer::GetBaseFormat(DXGI_FORMAT default_format)
{
  switch (default_format)
  {
  case DXGI_FORMAT_R8G8B8A8_UNORM:
  case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
    return DXGI_FORMAT_R8G8B8A8_TYPELESS;

  case DXGI_FORMAT_B8G8R8A8_UNORM:
  case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
    return DXGI_FORMAT_B8G8R8A8_TYPELESS;

  case DXGI_FORMAT_B8G8R8X8_UNORM:
  case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
    return DXGI_FORMAT_B8G8R8X8_TYPELESS;

    // 32-bit Z w/ Stencil
  case DXGI_FORMAT_R32G8X24_TYPELESS:
  case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
  case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
  case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
    return DXGI_FORMAT_R32G8X24_TYPELESS;

    // No Stencil
  case DXGI_FORMAT_R32_TYPELESS:
  case DXGI_FORMAT_D32_FLOAT:
  case DXGI_FORMAT_R32_FLOAT:
    return DXGI_FORMAT_R32_TYPELESS;

    // 24-bit Z
  case DXGI_FORMAT_R24G8_TYPELESS:
  case DXGI_FORMAT_D24_UNORM_S8_UINT:
  case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
  case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
    return DXGI_FORMAT_R24G8_TYPELESS;

    // 16-bit Z w/o Stencil
  case DXGI_FORMAT_R16_TYPELESS:
  case DXGI_FORMAT_D16_UNORM:
  case DXGI_FORMAT_R16_UNORM:
    return DXGI_FORMAT_R16_TYPELESS;

  default:
    return default_format;
  }
}

DXGI_FORMAT PixelBuffer::GetDepthFormat(DXGI_FORMAT default_format)
{
  switch (default_format)
  {
    // 32-bit Z w/ Stencil
  case DXGI_FORMAT_R32G8X24_TYPELESS:
  case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
  case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
  case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
    return DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;

    // No Stencil
  case DXGI_FORMAT_R32_TYPELESS:
  case DXGI_FORMAT_D32_FLOAT:
  case DXGI_FORMAT_R32_FLOAT:
    return DXGI_FORMAT_R32_FLOAT;

    // 24-bit Z
  case DXGI_FORMAT_R24G8_TYPELESS:
  case DXGI_FORMAT_D24_UNORM_S8_UINT:
  case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
  case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
    return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;

    // 16-bit Z w/o Stencil
  case DXGI_FORMAT_R16_TYPELESS:
  case DXGI_FORMAT_D16_UNORM:
  case DXGI_FORMAT_R16_UNORM:
    return DXGI_FORMAT_R16_UNORM;

  default:
    return DXGI_FORMAT_UNKNOWN;
  }
}

DXGI_FORMAT PixelBuffer::GetStencilFormat(DXGI_FORMAT default_format)
{
  switch (default_format)
  {
    // 32-bit Z w/ Stencil
  case DXGI_FORMAT_R32G8X24_TYPELESS:
  case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
  case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
  case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
    return DXGI_FORMAT_X32_TYPELESS_G8X24_UINT;

    // 24-bit Z
  case DXGI_FORMAT_R24G8_TYPELESS:
  case DXGI_FORMAT_D24_UNORM_S8_UINT:
  case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
  case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
    return DXGI_FORMAT_X24_TYPELESS_G8_UINT;

  default:
    return DXGI_FORMAT_UNKNOWN;
  }
}

DXGI_FORMAT PixelBuffer::GetUAVFormat( DXGI_FORMAT defaultFormat )
{
  switch (defaultFormat)
  {
  case DXGI_FORMAT_R8G8B8A8_TYPELESS:
  case DXGI_FORMAT_R8G8B8A8_UNORM:
  case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
    return DXGI_FORMAT_R8G8B8A8_UNORM;

  case DXGI_FORMAT_B8G8R8A8_TYPELESS:
  case DXGI_FORMAT_B8G8R8A8_UNORM:
  case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
    return DXGI_FORMAT_B8G8R8A8_UNORM;

  case DXGI_FORMAT_B8G8R8X8_TYPELESS:
  case DXGI_FORMAT_B8G8R8X8_UNORM:
  case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
    return DXGI_FORMAT_B8G8R8X8_UNORM;

  case DXGI_FORMAT_R32_TYPELESS:
  case DXGI_FORMAT_R32_FLOAT:
    return DXGI_FORMAT_R32_FLOAT;

#ifdef _DEBUG
  case DXGI_FORMAT_R32G8X24_TYPELESS:
  case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
  case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
  case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
  case DXGI_FORMAT_D32_FLOAT:
  case DXGI_FORMAT_R24G8_TYPELESS:
  case DXGI_FORMAT_D24_UNORM_S8_UINT:
  case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
  case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
  case DXGI_FORMAT_D16_UNORM:

    assert(false && "Requested a UAV format for a depth stencil format.");
#endif

  default:
    return defaultFormat;
  }
}