#pragma once

#ifndef PIXELBUFFER_H
#define PIXELBUFFER_H

#include "gpu_resource.h"
#include "descriptor_allocator.h"

class PixelBuffer : public GpuResource
{
public:
  PixelBuffer();
  ~PixelBuffer();

protected:
  void AssoiateWithResource(ID3D12Resource* resource, D3D12_RESOURCE_STATES state_usage);

  D3D12_RESOURCE_DESC DescribeTexture2D(uint32_t width, uint32_t height, DXGI_FORMAT format,
      uint32_t array_size, uint32_t mip_level, D3D12_RESOURCE_FLAGS flags);

  void CreateTextureResource(ID3D12Device* device, const std::wstring& name, D3D12_RESOURCE_DESC& descriptor, D3D12_CLEAR_VALUE& clear_value);

  DXGI_FORMAT GetBaseFormat(DXGI_FORMAT default_format);
  DXGI_FORMAT GetDepthFormat(DXGI_FORMAT default_format);
  DXGI_FORMAT GetStencilFormat(DXGI_FORMAT default_format);

  uint32_t width_;
  uint32_t height_;
  uint64_t array_size_;
  DXGI_FORMAT format_;

};

#endif // !PIXELBUFFER_H




