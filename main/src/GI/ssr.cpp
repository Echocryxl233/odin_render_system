#include "GI/ssr.h"
#include "GI/gi_utility.h"
#include "graphics/graphics_core.h"
#include "graphics/command_context.h"
#include "graphics/sampler_manager.h"
#include "game/camera.h"

namespace GI {

namespace Specular {

SSR MainSSR;

void SSR::Initialize(uint32_t width, uint32_t height) {
  OnResize(width, height);
  CreatePso();
}

void SSR::CreatePso() {
  SamplerDesc point_sampler;
  point_sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
  point_sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
  point_sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
  point_sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;

  SamplerDesc linear_sampler;
  linear_sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
  linear_sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
  linear_sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
  linear_sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;

  SamplerDesc depth_sampler;
  depth_sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
  depth_sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
  depth_sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
  depth_sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
  depth_sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

  SamplerDesc linear_wrap_sampler;
  linear_wrap_sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
  linear_wrap_sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
  linear_wrap_sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
  linear_wrap_sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;


  signature_.Reset(3, 4);
  signature_[0].InitAsConstantBufferView(0);
  signature_[1].InitAsConstants(1, 1);
  signature_[2].InitAsDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 3, 0, 0, D3D12_SHADER_VISIBILITY_PIXEL);
  signature_.InitSampler(0, point_sampler, D3D12_SHADER_VISIBILITY_PIXEL);
  signature_.InitSampler(1, linear_sampler, D3D12_SHADER_VISIBILITY_PIXEL);
  signature_.InitSampler(2, depth_sampler, D3D12_SHADER_VISIBILITY_PIXEL);
  signature_.InitSampler(3, linear_wrap_sampler, D3D12_SHADER_VISIBILITY_PIXEL);
  signature_.Finalize();

  auto ssr_vs = d3dUtil::CompileShader(L"shaders/ssr.hlsl", nullptr, "VS", "vs_5_1");
  auto ssr_ps = d3dUtil::CompileShader(L"shaders/ssr.hlsl", nullptr, "PS", "ps_5_1");

  std::vector<D3D12_INPUT_ELEMENT_DESC> input_layout =
  {
    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    //  { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
  };

  pso_.SetInputLayout(input_layout.data(), (UINT)input_layout.size());
  pso_.SetVertexShader(reinterpret_cast<BYTE*>(ssr_vs->GetBufferPointer()),
    (UINT)ssr_vs->GetBufferSize());
  pso_.SetPixelShader(reinterpret_cast<BYTE*>(ssr_ps->GetBufferPointer()),
    (UINT)ssr_ps->GetBufferSize());

  pso_.SetRootSignature(signature_);
  pso_.SetBlendState(CD3DX12_BLEND_DESC(D3D12_DEFAULT));
  pso_.SetRasterizeState(CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT));
  pso_.SetDepthStencilState(Graphics::DepthStateDisabled);
  pso_.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
  pso_.SetSampleMask(UINT_MAX);
  pso_.SetRenderTargetFormat(GI::kNormalMapFormat, DXGI_FORMAT_UNKNOWN);
  pso_.Finalize();
}

void SSR::OnResize(uint32_t width, uint32_t height) {

  if (width != width_ || height != height_) {
    width_ = width;
    height_ = height;

    color_map_.Create(L"ssr_target", width_, height_, 1, GI::kNormalMapFormat);
    normal_map_.Create(L"ssr_normal", width_, height_, 1, GI::kNormalMapFormat);
    depth_buffer_.Create(L"ssr_depth", width_, height_, Graphics::Core.DepthStencilFormat);
  }

}

void SSR::DrawNormalAndDepth() {
  GraphicsContext& context = GraphicsContext::Begin(L"ssr_context");
  

  context.Finish(true);
}

void SSR::RenderReflection() {
  //GI::Specular::MainSSR.DrawNormalAndDepth();

  GraphicsContext& context = GraphicsContext::Begin(L"ssr_context");

  vector<RenderObject*> objects;
  auto begin = Graphics::MainQueue.Begin();
  for (auto it = begin; it != Graphics::MainQueue.End(); ++it) {

    auto group_begin = it->second->Begin();
    auto group_end = it->second->End();
    for (auto it_obj = group_begin; it_obj != group_end; ++it_obj) {
      objects.push_back((*it_obj));
    }
  }

  GI::Utility::DrawNormalAndDepthMap(context, normal_map_, depth_buffer_, objects);

  //ColorBuffer* buffer = &ambient_map_0_;

  //context.SetViewports(&viewport_);
  //context.SetScissorRects(&scissor_rect_);

  context.TransitionResource(color_map_, D3D12_RESOURCE_STATE_RENDER_TARGET, true);
  context.ClearColor(color_map_);
  context.SetRenderTarget(&color_map_.Rtv());

  context.SetRootSignature(signature_);
  context.SetPipelineState(pso_);

  XMFLOAT4X4& view = GameCore::MainCamera.View4x4f();
  XMFLOAT4X4& proj = GameCore::MainCamera.Proj4x4f();

  XMMATRIX view_matrix = XMLoadFloat4x4(&proj);
  XMMATRIX inv_view_matrix = XMMatrixInverse(&XMMatrixDeterminant(view_matrix), view_matrix);

  XMMATRIX proj_matrix = XMLoadFloat4x4(&proj);
  XMMATRIX inv_proj_matrix = XMMatrixInverse(&XMMatrixDeterminant(proj_matrix), proj_matrix);



  __declspec(align(16))
    struct SSRConstants
  {
    XMFLOAT4X4 View;
    XMFLOAT4X4 InvView;
    XMFLOAT4X4 Proj;
    XMFLOAT4X4 InvProj;
  };

  SSRConstants constants;

  XMStoreFloat4x4(&constants.View, XMMatrixTranspose(view_matrix));
  XMStoreFloat4x4(&constants.InvView, XMMatrixTranspose(inv_view_matrix));

  XMStoreFloat4x4(&constants.Proj, XMMatrixTranspose(proj_matrix));
  XMStoreFloat4x4(&constants.InvProj, XMMatrixTranspose(inv_proj_matrix));

  context.SetDynamicConstantBufferView(0, sizeof(constants), &constants);

  //  context.SetRoot32BitConstant(1, 1, 0);

  D3D12_CPU_DESCRIPTOR_HANDLE handles[] = { normal_map_.Srv(), depth_buffer_.DepthSRV(), Graphics::Core.DisplayPlane().Srv() };
  context.SetDynamicDescriptors(2, 0, _countof(handles), handles);

  context.SetVertexBuffers(0, 0, nullptr);
  context.SetNullIndexBuffer();
  context.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  context.DrawInstanced(6, 1, 0, 0);
  context.TransitionResource(color_map_, D3D12_RESOURCE_STATE_GENERIC_READ, true);
  context.Finish(true);
}

};

};