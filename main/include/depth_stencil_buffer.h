#pragma once

#ifndef DEPTHSTENCILBUFFER_H
#define DEPTHSTENCILBUFFER_H


#include "pixel_buffer.h"
#include "utility.h"

class DepthStencilBuffer : public PixelBuffer {
 public:
  DepthStencilBuffer(float depth = 1.0f, uint8_t stencil = 0);
  void Create(ID3D12Device* device, uint32_t width, uint32_t height, DXGI_FORMAT format);
  
  D3D12_CPU_DESCRIPTOR_HANDLE DepthSRV() const { return depth_srv_; }
  D3D12_CPU_DESCRIPTOR_HANDLE StencilSRV() const { return stencil_srv_; }
  D3D12_CPU_DESCRIPTOR_HANDLE DSV() const { return dsv_[0]; }
  D3D12_CPU_DESCRIPTOR_HANDLE DSVDepthReadOnly() const { return dsv_[1]; }
  D3D12_CPU_DESCRIPTOR_HANDLE DSVStencilReadOnly() const { return dsv_[2]; }
  D3D12_CPU_DESCRIPTOR_HANDLE DSVReadOnly() const { return dsv_[3]; }

  float ClearDepth() const { return depth_; }
  uint8_t ClearStencil() const { return stencil_; }

 private:
  void CreateDeriveView(ID3D12Device* device, DXGI_FORMAT format);

 private:
  float depth_;
  uint8_t stencil_;

  D3D12_CPU_DESCRIPTOR_HANDLE depth_srv_;
  D3D12_CPU_DESCRIPTOR_HANDLE stencil_srv_;
  D3D12_CPU_DESCRIPTOR_HANDLE dsv_[4];
};

#endif // !DEPTHSTENCILBUFFER_H

