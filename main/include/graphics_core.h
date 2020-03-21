#pragma once

#ifndef GRAPHICSCORE_H
#define GRAPHICSCORE_H


#include "color_buffer.h"
#include "depth_stencil_buffer.h"
#include "linear_allocator.h"
#include "graphics_common.h"

#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "dxgi.lib")

namespace Graphics {

using namespace std;
using namespace Microsoft::WRL;

class GraphicsCore {
 public:
  ~GraphicsCore();

  void Initialize(HWND h_window_);
  void OnResize(UINT width, UINT height);
  void PreparePresent();
  void Present();

  ID3D12Device* Device() { return device_.Get(); }
  ColorBuffer& DisplayPlane() { return display_planes_[current_back_buffer_index_]; }
  DepthStencilBuffer& DepthBuffer() { return depth_buffer_; }

  D3D12_VIEWPORT& ViewPort() { return screen_viewport_; }
  D3D12_RECT& ScissorRect() { return scissor_rect_; }

  D3D12_CPU_DESCRIPTOR_HANDLE* MrtRtvs() { return mrt_rtvs_; }
  ColorBuffer& GetMrtBuffer(int i) { return mrt_buffers_[i]; }
  DXGI_FORMAT* MrtFormats() { return mrt_formats_; }
  DepthStencilBuffer& DeferredDepthBuffer() { return deferred_depth_buffer_; }

  long Width() const { return client_width_; }
  long Height() const { return client_height_; }
  
  const static int kSwapBufferCount = 2;
  const static int kMRTBufferCount = 4;

  DXGI_FORMAT BackBufferFormat = DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT;
  DXGI_FORMAT DepthStencilFormat = DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT;

  DXGI_FORMAT MrtBufferFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;

  bool Msaa4xState() const { return msaa_4x_state_; }
  UINT Msaa4xQuanlity() const { return msaa_4x_quanlity_; }
  
 private:
  void CreateSwapChain(HWND h_window_);
  void LogAdapters();
  void LogAdapterOutputs(IDXGIAdapter* adapter);
  void LogOutputDisplayModes(IDXGIOutput* output, DXGI_FORMAT format);
  void CreateDisplayPlanes();
  void CreateViewportAndRect();
  
  void CreateMRTBuffers();

 private:

  ComPtr<IDXGIFactory4> dxgi_factory;
  ComPtr<ID3D12Device> device_;
  ComPtr<IDXGISwapChain> swap_chain_;

  long client_width_ = 800;
  long client_height_ = 600;

  ColorBuffer display_planes_[kSwapBufferCount]; 
  DepthStencilBuffer depth_buffer_;
  int current_back_buffer_index_;

  D3D12_VIEWPORT screen_viewport_;
  D3D12_RECT scissor_rect_;

  bool msaa_4x_state_ = false;
  UINT msaa_4x_quanlity_ = 0;

  ColorBuffer mrt_buffers_[kMRTBufferCount];
  D3D12_CPU_DESCRIPTOR_HANDLE mrt_rtvs_[kMRTBufferCount];
  DXGI_FORMAT mrt_formats_[kMRTBufferCount];
  DepthStencilBuffer deferred_depth_buffer_;
};

extern GraphicsCore Core;

};

#endif // !GRAPHICSCORE_H

