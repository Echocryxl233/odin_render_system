#pragma once

#ifndef COLORBUFFER_H
#define COLORBUFFER_H

#include <string>
#include "color.h"
#include "resource/pixel_buffer.h"

class ColorBuffer : public PixelBuffer
{
 public:
  ColorBuffer(Color color = Color(1.0f, 1.0f, 1.0f, 1.0f));
  ~ColorBuffer();

  void CreateFromSwapChain(std::wstring& name, ID3D12Device* device, ID3D12Resource* resouce);
  void Create(wstring name, uint32_t width, uint32_t height, uint32_t mips_count, DXGI_FORMAT format);

  const D3D12_CPU_DESCRIPTOR_HANDLE& Rtv() { return rtv_handle_; } 
  const D3D12_CPU_DESCRIPTOR_HANDLE& Srv() { return srv_handle_; } 
  const D3D12_CPU_DESCRIPTOR_HANDLE& Uav() { return uav_handle_[0]; } 

  //  const D3D12_CPU_DESCRIPTOR_HANDLE& Srv() { return srv_handle_; } 

  Color GetColor() const { return color_; }
  void SetColor(const Color& color) { color_ = color; }

  uint32_t MipCount() const { return resource_->GetDesc().MipLevels; }

  //  D3D12_CPU_DESCRIPTOR_HANDLE GetDescriptorHandle() { return descriptor_handle_; }

 private:
  void CreateDeriveView(DXGI_FORMAT format, uint32_t mips_count);

  D3D12_RESOURCE_FLAGS ColorBuffer::CombineFlags() {

    D3D12_RESOURCE_FLAGS combined_flags = D3D12_RESOURCE_FLAG_NONE;
    if (D3D12_RESOURCE_FLAG_NONE == combined_flags && fragment_count_ == 1) {
      combined_flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    }
    return combined_flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
  }

  static inline uint32_t ComputeNumMips(uint32_t Width, uint32_t Height)
  {
    uint32_t high_bit;
    _BitScanReverse((unsigned long*)&high_bit, Width | Height);
    return high_bit + 1;
  }
  // D3D12_CPU_DESCRIPTOR_HANDLE descriptor_handle_;    //  the location of the resource in the descriptor heap

  D3D12_CPU_DESCRIPTOR_HANDLE rtv_handle_;
  D3D12_CPU_DESCRIPTOR_HANDLE srv_handle_;
  D3D12_CPU_DESCRIPTOR_HANDLE uav_handle_[12];

  Color color_;
  uint32_t fragment_count_;
  uint32_t mips_count_;
  bool create_from_swapchain_flag_;
};

#endif // !COLORBUFFER_H



