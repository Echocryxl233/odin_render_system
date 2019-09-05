#pragma once

#ifndef SSAO_H
#define SSAO_H

#include "color_buffer.h"
#include "command_context.h"
#include "pipeline_state.h"
#include "gpu_buffer.h"

class Ssao {
 public :
  void Initialize(UINT width, UINT height);

  void BeginRender(GraphicsContext& context, XMFLOAT4X4& view, XMFLOAT4X4& proj);
  void EndRender(GraphicsContext& context);

  ColorBuffer& NormalMap() { return normal_map_; }

 private:
  RootSignature root_signature_;
  GraphicsPso   draw_normal_pso_;

  ColorBuffer normal_map_;
  ColorBuffer ambient_map_0_;
  ColorBuffer ambient_map_1_;

  UINT width_;
  UINT height_;

  D3D12_VIEWPORT viewport_;
  D3D12_RECT scissor_rect_;

  static const DXGI_FORMAT kNormalMapFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;


};

#endif // !SSAO_H

