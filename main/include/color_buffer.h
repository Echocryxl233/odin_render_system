#pragma once

#ifndef COLORBUFFER_H
#define COLORBUFFER_H

#include <string>
#include "color.h"
#include "pixel_buffer.h"

class ColorBuffer : public PixelBuffer
{
 public:
  ColorBuffer(Color color = Color(1.0f, 1.0f, 1.0f, 1.0f));
  ~ColorBuffer();

  void CreateFromSwapChain(std::wstring& name, ID3D12Device* device, ID3D12Resource* resouce);
  void Create(wstring name, uint32_t width, uint32_t height, DXGI_FORMAT format, D3D12_RESOURCE_FLAGS flags);
  //  void CreateButNotRTV(wstring name, uint32_t width, uint32_t height, DXGI_FORMAT format, D3D12_RESOURCE_FLAGS flags);

  const D3D12_CPU_DESCRIPTOR_HANDLE& Rtv() { return rtv_handle_; } 
  const D3D12_CPU_DESCRIPTOR_HANDLE& Srv() { return srv_handle_; } 

  Color GetColor() const { return color_; }
  void SetColor(const Color& color) { color_ = color; }

  //  D3D12_CPU_DESCRIPTOR_HANDLE GetDescriptorHandle() { return descriptor_handle_; }

 private:
  void CreateDeriveView(DXGI_FORMAT format);
  // D3D12_CPU_DESCRIPTOR_HANDLE descriptor_handle_;    //  the location of the resource in the descriptor heap

  D3D12_CPU_DESCRIPTOR_HANDLE rtv_handle_;
  D3D12_CPU_DESCRIPTOR_HANDLE srv_handle_;

  Color color_;
};

#endif // !COLORBUFFER_H



