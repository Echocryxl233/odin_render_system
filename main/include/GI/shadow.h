#pragma once

#ifndef GI_SHADOW_H
#define GI_SHADOW_H

#include "utility.h"
#include "color_buffer.h"
#include "depth_stencil_buffer.h"
#include "pipeline_state.h"

namespace GI {

namespace Shadow {

class ShadowMap {

public:
  void Initialize(uint32_t width, uint32_t height);
  void Render();
  void Update();

  DepthStencilBuffer& DepthBuffer() { return depth_buffer_; }
  XMFLOAT4X4 ShadowTransform() const { return shadow_transform_;}

private:

private:
  DepthStencilBuffer depth_buffer_;
  ColorBuffer color_buffer_;
  D3D12_VIEWPORT viewport_;
  D3D12_RECT scissor_rect_;

  UINT width_ = 0;
  UINT height_ = 0;


  RootSignature signature_;
  GraphicsPso pso_;

  DirectX::BoundingSphere scene_bounds_;

  XMMATRIX light_view_;
  XMMATRIX light_proj_;

  XMFLOAT4X4 shadow_transform_;

};

extern ShadowMap MainShadow;
}

};

#endif // !GI_SHADOW_H

