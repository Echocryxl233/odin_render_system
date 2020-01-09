#pragma once

#ifndef SSAO_H
#define SSAO_H

#include "color_buffer.h"
#include "command_context.h"
#include "pipeline_state.h"
#include "gpu_buffer.h"


using namespace std;
using namespace DirectX;

namespace GI {

namespace AO {

class Ssao {
public:
  const XMMATRIX kTextureTransform = {
    0.5f, 0.0f, 0.0f, 0.0f,
    0.0f, -0.5f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.5f, 0.5f, 0.0f, 1.0f,
  };

public:

  void Initialize(UINT width, UINT height);

  void Resize(UINT width, UINT height);

  void BeginRender(GraphicsContext& context, XMFLOAT4X4& view, XMFLOAT4X4& proj);
  void EndRender(GraphicsContext& context);

  void ComputeAo();

  

  ColorBuffer& NormalMap() { return normal_map_; }
  ColorBuffer& AmbientMap() { return ambient_map_0_; }
  ColorBuffer& AmbientMap1() { return ambient_map_1_; }


  DepthStencilBuffer& DepthMap() { return depth_buffer_; }

  ColorBuffer& RandomMap() { return random_map_; }

private:
  void BuildDrawNormalComponents();
  void BuildAmbientComponents();

  void BlurAmbientMap(GraphicsContext& context, bool is_horizontal);
  void ComputeAoInternal(GraphicsContext& context, XMFLOAT4X4& view, XMFLOAT4X4& proj);

  vector<float> GetGaussWeight(float sigma);

  void BuildOffsetVectors();

private:
  RootSignature draw_normal_root_signature_;
  GraphicsPso   draw_normal_pso_;

  RootSignature ssao_root_signature_;
  RootSignature ssao_root_signature_1;
  GraphicsPso   ssao_pso_;

  RootSignature blur_root_signature_;
  GraphicsPso   blur_pso_;



  ColorBuffer normal_map_;
  ColorBuffer normal_map_1;
  ColorBuffer ambient_map_0_;
  ColorBuffer ambient_map_1_;
  DepthStencilBuffer depth_buffer_;
  ColorBuffer random_map_;

  UINT width_;
  UINT height_;

  D3D12_VIEWPORT viewport_;
  D3D12_RECT scissor_rect_;

  XMFLOAT4 offsets_[14];

  static const DXGI_FORMAT kNormalMapFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
  static const DXGI_FORMAT kAmbientMapFormat = DXGI_FORMAT_R16_UNORM;


  // Transform NDC space [-1,+1]^2 to texture space [0,1]^2
  // NDC space is bottom to top, and texture space is up to down


  __declspec(align(16))
    struct SsaoConstants
  {
    XMFLOAT4X4 Proj;
    XMFLOAT4X4 InvProj;
    XMFLOAT4X4 ProjTex;
    XMFLOAT4   OffsetVectors[14];

    // For SsaoBlur.hlsl
    XMFLOAT4 BlurWeights[3];


    XMFLOAT2 InvRenderTargetSize = { 0.0f, 0.0f };
    float padding1;
    float padding2;

    // Coordinates given in view space.
    float OcclusionRadius = 0.5f;
    float OcclusionFadeStart = 0.2f;
    float OcclusionFadeEnd = 2.0f;
    float SurfaceEpsilon = 0.05f;
  };

  struct SsaoConstants2
  {
    XMFLOAT4X4 Proj;
    XMFLOAT4X4 InvProj;
    XMFLOAT4X4 ProjTex;
    XMFLOAT4   OffsetVectors[14];

    // For SsaoBlur.hlsl
    XMFLOAT4 BlurWeights[3];

    XMFLOAT2 InvRenderTargetSize = { 0.0f, 0.0f };
    float padding1;
    float padding2;

    // Coordinates given in view space.
    float OcclusionRadius = 0.5f;
    float OcclusionFadeStart = 0.2f;
    float OcclusionFadeEnd = 2.0f;
    float SurfaceEpsilon = 0.05f;
  };

  SsaoConstants ssao_constants_;
};

extern Ssao MainSsao;

}
};



#endif // !SSAO_H

