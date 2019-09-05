#include "graphics_core.h"
#include "ssao.h"
#include "sampler_manager.h"
#include "command_context.h"
#include "utility.h"

void Ssao::Initialize(UINT width, UINT height) {
  width_ = width;
  height_ = height;

  viewport_.TopLeftX = 0.0f;
  viewport_.TopLeftY = 0.0f;
  viewport_.Width = width / 2.0f;
  viewport_.Height = height / 2.0f;
  viewport_.MinDepth = 0.0f;
  viewport_.MaxDepth = 1.0f;

  scissor_rect_ = { 0, 0, (int)width / 2, (int)height / 2 };

  //CD3DX12_DESCRIPTOR_RANGE descriptor_range1;
  //descriptor_range1.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 0, 0);


  //CD3DX12_DESCRIPTOR_RANGE descriptor_range2;
  //descriptor_range2.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2, 0);

  //SamplerDesc default_sampler;

  root_signature_.Reset(2, 0);
  root_signature_[0].InitAsConstantBufferView(0);
  root_signature_[1].InitAsConstantBufferView(1);
  //root_signature_[1].InitAsConstants(1, 1);
  //root_signature_[2].InitAsDescriptorTable(1, &descriptor_range1, D3D12_SHADER_VISIBILITY_PIXEL);
  //root_signature_[3].InitAsDescriptorTable(1, &descriptor_range2, D3D12_SHADER_VISIBILITY_PIXEL);
  //root_signature_.InitSampler(0, default_sampler, D3D12_SHADER_VISIBILITY_PIXEL);

  root_signature_.Finalize();

  auto ssao_normal_vs = d3dUtil::CompileShader(L"shaders/ssao_normal.hlsl", nullptr, "VS", "vs_5_1");
  auto ssao_normal_ps = d3dUtil::CompileShader(L"shaders/ssao_normal.hlsl", nullptr, "PS", "ps_5_1");

  std::vector<D3D12_INPUT_ELEMENT_DESC> input_layout =
  {
    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    //  { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
  };

  draw_normal_pso_.SetInputLayout(input_layout.data(), (UINT)input_layout.size());
  draw_normal_pso_.SetVertexShader(reinterpret_cast<BYTE*>(ssao_normal_vs->GetBufferPointer()),
    (UINT)ssao_normal_vs->GetBufferSize());
  draw_normal_pso_.SetPixelShader(reinterpret_cast<BYTE*>(ssao_normal_ps->GetBufferPointer()),
    (UINT)ssao_normal_ps->GetBufferSize());

  draw_normal_pso_.SetRootSignature(root_signature_);
  draw_normal_pso_.SetBlendState(CD3DX12_BLEND_DESC(D3D12_DEFAULT));
  draw_normal_pso_.SetRasterizeState(CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT));
  draw_normal_pso_.SetDepthStencilState(CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT));
  draw_normal_pso_.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
  draw_normal_pso_.SetSampleMask(UINT_MAX);
  draw_normal_pso_.SetRenderTargetFormat(Ssao::kNormalMapFormat, Graphics::Core.DepthStencilFormat);
  draw_normal_pso_.Finalize(Graphics::Core.Device());

  normal_map_.Create(width_, height_, Ssao::kNormalMapFormat, 
      D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);

  //normal_map_.SetColor(Color(0.0f, 1.0f, 1.0f, 0.0f));

  //ambient_map_0_.Create(width_/2, height_/2, DXGI_FORMAT_R16_UNORM, 
  //    D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);

  //ambient_map_1_.Create(width_ / 2, height_ / 2, DXGI_FORMAT_R16_UNORM,
  //  D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);


}

void Ssao::BeginRender(GraphicsContext& context, XMFLOAT4X4& view, XMFLOAT4X4& proj) {

  context.SetViewports(&Graphics::Core.ViewPort());
  context.SetScissorRects(&Graphics::Core.ScissorRect());

  context.TransitionResource(normal_map_, D3D12_RESOURCE_STATE_RENDER_TARGET, true);

  context.ClearColor(normal_map_);
  context.ClearDepthStencil(Graphics::Core.DepthBuffer());
  context.SetRenderTargets(1, &normal_map_.GetRTV(), Graphics::Core.DepthBuffer().DSV());
  context.SetRootSignature(root_signature_);
  context.SetPipelineState(draw_normal_pso_);
  
  __declspec(align(16)) 
  struct SsaoVsConstant {
    XMFLOAT4X4 proj;
    XMFLOAT4X4 view_proj;
  } ssao_vs_constant;  
  
  XMMATRIX view_matrix = XMLoadFloat4x4(&view);
  XMMATRIX proj_matrix = XMLoadFloat4x4(&proj);
  XMMATRIX view_proj_matrix = XMMatrixMultiply(view_matrix, proj_matrix);
  
  XMStoreFloat4x4(&ssao_vs_constant.proj, XMMatrixTranspose(proj_matrix));
  XMStoreFloat4x4(&ssao_vs_constant.view_proj, XMMatrixTranspose(view_proj_matrix));

  context.SetDynamicConstantBufferView(1, sizeof(ssao_vs_constant), &ssao_vs_constant);
}

void Ssao::EndRender(GraphicsContext& context) {
  context.TransitionResource(normal_map_, D3D12_RESOURCE_STATE_GENERIC_READ, true);
}

