#include "optional_system.h"
#include "graphics_core.h"
#include "graphics_common.h"
#include "mesh_geometry.h"
#include "model.h"
#include "skybox.h"
#include "ssao.h"
#include "game_setting.h"

namespace Graphics {

using namespace Geometry;

void OptionalSystem::SetRenderQueue(RenderQueue* queue) {
  render_queue_ = queue;
}

void OptionalSystem::Render(GraphicsContext& context) {
  OnRender(context);

  OnOptionalPass(context);
  //context.Flush();
}

void ForwardShading::Initialize() {
  D3D_SHADER_MACRO ssao_macro[] = {
    "SSAO", "1", NULL, NULL
  };

  D3D_SHADER_MACRO* macro = GameSetting::GetBoolValue("UseSsao") ? ssao_macro : nullptr;;

  auto color_vs = d3dUtil::CompileShader(L"shaders/color_standard.hlsl", macro, "VS", "vs_5_1");
  auto color_ps = d3dUtil::CompileShader(L"shaders/color_standard.hlsl", nullptr, "PS", "ps_5_1");

  std::vector<D3D12_INPUT_ELEMENT_DESC> input_layout =
  {
      { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
      { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
      { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
  };

  SamplerDesc default_sampler;

  root_signature_.Reset(6, 1);
  root_signature_.InitSampler(0, default_sampler, D3D12_SHADER_VISIBILITY_PIXEL);
  root_signature_[0].InitAsConstantBufferView(0, 0);
  root_signature_[1].InitAsConstantBufferView(1, 0);
  root_signature_[2].InitAsConstantBufferView(2, 0);
  root_signature_[3].InitAsDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 0, 0, D3D12_SHADER_VISIBILITY_PIXEL);
  root_signature_[4].InitAsDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 2, D3D12_SHADER_VISIBILITY_PIXEL);
  root_signature_[5].InitAsDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, 2, D3D12_SHADER_VISIBILITY_PIXEL);
  root_signature_.Finalize();

  graphics_pso_.SetInputLayout(input_layout.data(), (UINT)input_layout.size());
  graphics_pso_.SetVertexShader(reinterpret_cast<BYTE*>(color_vs->GetBufferPointer()),
    (UINT)color_vs->GetBufferSize());
  graphics_pso_.SetPixelShader(reinterpret_cast<BYTE*>(color_ps->GetBufferPointer()),
    (UINT)color_ps->GetBufferSize());

  graphics_pso_.SetRootSignature(root_signature_);
  graphics_pso_.SetRasterizeState(CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT));
  graphics_pso_.SetBlendState(BlendAdditional);  // CD3DX12_BLEND_DESC(D3D12_DEFAULT)
  graphics_pso_.SetDepthStencilState(CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT));
  graphics_pso_.SetSampleMask(UINT_MAX);
  graphics_pso_.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
  graphics_pso_.SetRenderTargetFormat(Graphics::Core.BackBufferFormat, Graphics::Core.DepthStencilFormat,
    (Graphics::Core.Msaa4xState() ? 4 : 1), (Graphics::Core.Msaa4xState() ? (Graphics::Core.Msaa4xQuanlity() - 1) : 0));
  graphics_pso_.Finalize();
}

void ForwardShading::OnRender(GraphicsContext& context) {
  auto& display_plane = Graphics::Core.DisplayPlane();

  //context.SetViewports(&Graphics::Core.ViewPort());
  //context.SetScissorRects(&Graphics::Core.ScissorRect());
  context.TransitionResource(display_plane, D3D12_RESOURCE_STATE_RENDER_TARGET, true);

  //  context.ClearColor(display_plane);
  context.ClearDepthStencil(Graphics::Core.DepthBuffer());
  context.SetRenderTargets(1, &Graphics::Core.DisplayPlane().Rtv(), Graphics::Core.DepthBuffer().DSV());

  context.SetPipelineState(graphics_pso_);
  context.SetRootSignature(root_signature_);

  context.SetDynamicDescriptors(4, 0, 1, &Graphics::SkyBox::SkyBoxTexture->SrvHandle());
  auto& ssao_ambient_handle = const_cast<D3D12_CPU_DESCRIPTOR_HANDLE&>(GI::AO::MainSsao.AmbientMap().Srv());
  context.SetDynamicDescriptors(5, 0, 1, &ssao_ambient_handle);
  auto begin = render_queue_->Begin();

  for (auto it = begin; it != render_queue_->End(); ++it) {

    auto group_begin = it->second->Begin();
    auto group_end = it->second->End();
    for (auto it_obj = group_begin; it_obj != group_end; ++it_obj) {
      (*it_obj)->Render(context);
    }
  }
}

void DeferredBase::Initialize() {
  //  Graphics::CreateQuad(-1.0f, 1.0f, 2.0f, 2.0f, 0.0f, vertex_buffer_, index_buffer_);
  mesh_quad_.CreateQuad(-1.0f, 1.0f, 2.0f, 2.0f, 0.0f);
}

void DeferredShading::Initialize() {
  DeferredBase::Initialize();
  auto deferred_pass1_vs = d3dUtil::CompileShader(L"shaders/deferred_shading_pass1.hlsl", nullptr, "VS", "vs_5_1");
  auto deferred_pass1_ps = d3dUtil::CompileShader(L"shaders/deferred_shading_pass1.hlsl", nullptr, "PS", "ps_5_1");

  std::vector<D3D12_INPUT_ELEMENT_DESC> input_layout =
  {
    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
  };

  SamplerDesc default_sampler;
  pass1_signature_.Reset(4, 1);
  pass1_signature_.InitSampler(0, default_sampler, D3D12_SHADER_VISIBILITY_PIXEL);
  pass1_signature_[0].InitAsConstantBufferView(0, 0);
  pass1_signature_[1].InitAsConstantBufferView(1, 0);
  pass1_signature_[2].InitAsConstantBufferView(2, 0);
  pass1_signature_[3].InitAsDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_SHADER_VISIBILITY_PIXEL);
  pass1_signature_.Finalize();

  pass1_pso_.SetInputLayout(input_layout.data(), (UINT)input_layout.size());
  pass1_pso_.SetVertexShader(deferred_pass1_vs);
  pass1_pso_.SetPixelShader(deferred_pass1_ps);


  pass1_pso_.SetRootSignature(pass1_signature_);
  pass1_pso_.SetRasterizeState(CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT));
  pass1_pso_.SetBlendState(CD3DX12_BLEND_DESC(D3D12_DEFAULT));
  pass1_pso_.SetDepthStencilState(CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT));
  pass1_pso_.SetSampleMask(UINT_MAX);
  pass1_pso_.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
  pass1_pso_.SetRenderTargetFormats(Graphics::Core.kMRTBufferCount, Graphics::Core.MrtFormats(), 
    Graphics::Core.DepthStencilFormat, (Graphics::Core.Msaa4xState() ? 4 : 1), 
    (Graphics::Core.Msaa4xState() ? (Graphics::Core.Msaa4xQuanlity() - 1) : 0));
  pass1_pso_.Finalize();

  //  -----------------
  SamplerDesc depth_sampler;
  depth_sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
  depth_sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
  depth_sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
  depth_sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
  depth_sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
  pass2_signature_.Reset(4, 2);
  pass2_signature_.InitSampler(0, default_sampler, D3D12_SHADER_VISIBILITY_PIXEL);
  pass2_signature_.InitSampler(1, depth_sampler, D3D12_SHADER_VISIBILITY_PIXEL);
  pass2_signature_[0].InitAsConstantBufferView(0, 0);
  pass2_signature_[1].InitAsConstantBufferView(1, 0);
  pass2_signature_[2].InitAsConstantBufferView(2, 0);
  pass2_signature_[3].InitAsDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4, 0, 0, D3D12_SHADER_VISIBILITY_PIXEL);
  pass2_signature_.Finalize();

  auto deferred_pass2_vs = d3dUtil::CompileShader(L"shaders/deferred_shading_pass2.hlsl", nullptr, "VS", "vs_5_1");
  auto deferred_pass2_ps = d3dUtil::CompileShader(L"shaders/deferred_shading_pass2.hlsl", nullptr, "PS", "ps_5_1");

  pass2_pso_.SetInputLayout(input_layout.data(), (UINT)input_layout.size());

  pass2_pso_.SetVertexShader(deferred_pass2_vs);
  pass2_pso_.SetPixelShader(deferred_pass2_ps);

  pass2_pso_.SetRootSignature(pass2_signature_);
  pass2_pso_.SetRasterizeState(CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT));
  pass2_pso_.SetBlendState(CD3DX12_BLEND_DESC(D3D12_DEFAULT));
  //  pass2_pso_.SetDepthStencilState(CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT));
  pass2_pso_.SetSampleMask(UINT_MAX);
  pass2_pso_.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
  pass2_pso_.SetRenderTargetFormats(1, &Graphics::Core.BackBufferFormat,
    //  Graphics::Core.DepthStencilFormat, (Graphics::Core.Msaa4xState() ? 4 : 1),
    DXGI_FORMAT::DXGI_FORMAT_UNKNOWN, (Graphics::Core.Msaa4xState() ? 4 : 1),
    (Graphics::Core.Msaa4xState() ? (Graphics::Core.Msaa4xQuanlity() - 1) : 0));
  pass2_pso_.Finalize();  
  
}

void DeferredShading::OnRender(GraphicsContext& context) {
  auto& display_plane = Graphics::Core.DisplayPlane();
//  pass1 start
  //context.SetViewports(&Graphics::Core.ViewPort());
  //context.SetScissorRects(&Graphics::Core.ScissorRect());

  for (int i=0; i< Graphics::Core.kMRTBufferCount; ++i) {
    context.TransitionResource(Graphics::Core.GetMrtBuffer(i), D3D12_RESOURCE_STATE_RENDER_TARGET, true);
    context.ClearColor(Graphics::Core.GetMrtBuffer(i));
  }

  context.ClearDepthStencil(Graphics::Core.DepthBuffer());
  context.SetRenderTargets(Graphics::Core.kMRTBufferCount, 
    Graphics::Core.MrtRtvs(), Graphics::Core.DepthBuffer().DSV());

  context.SetPipelineState(pass1_pso_);
  context.SetRootSignature(pass1_signature_);
  
  auto begin = render_queue_->Begin();
  for (auto it = begin; it != render_queue_->End(); ++it) {
    if (it->first == Graphics::RenderGroupType::RenderGroupType::kTransparent)
      continue;

    auto group_begin = it->second->Begin();
    auto group_end = it->second->End();
    for (auto it_obj = group_begin; it_obj != group_end; ++it_obj) {
      (*it_obj)->Render(context);
    }
  }

//  pass1 finish

  context.CopyBuffer(Graphics::Core.DeferredDepthBuffer(), Graphics::Core.DepthBuffer());
  context.TransitionResource(Graphics::Core.DepthBuffer(), D3D12_RESOURCE_STATE_DEPTH_WRITE);

  context.TransitionResource(display_plane, D3D12_RESOURCE_STATE_RENDER_TARGET, true);
  //  context.ClearColor(display_plane);

//  pass2 start

  context.SetRenderTarget(&display_plane.Rtv());

  context.SetPipelineState(pass2_pso_);
  context.SetRootSignature(pass2_signature_);

  context.SetDynamicConstantBufferView(2, sizeof(Graphics::MainConstants), &Graphics::MainConstants);

  D3D12_CPU_DESCRIPTOR_HANDLE handles[Graphics::Core.kMRTBufferCount];

  for (int i = 0; i < Graphics::Core.kMRTBufferCount-1; ++i) {
    handles[i] = Graphics::Core.GetMrtBuffer(i).Srv();
  }
  context.TransitionResource(Graphics::Core.DeferredDepthBuffer(), D3D12_RESOURCE_STATE_GENERIC_READ);
  handles[Graphics::Core.kMRTBufferCount-1] = Graphics::Core.DeferredDepthBuffer().DepthSRV();

  context.SetDynamicDescriptors(3, 0, _countof(handles), handles);

  context.SetVertexBuffer(mesh_quad_.VertexBuffer().VertexBufferView());
  context.SetIndexBuffer(mesh_quad_.IndexBuffer().IndexBufferView());
  context.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  context.DrawIndexedInstanced(mesh_quad_.IndexBuffer().ElementCount(), 1, 0, 0, 0);
}


void DeferredLighting::Initialize() {
  DeferredBase::Initialize();
  auto deferred_pass1_vs = d3dUtil::CompileShader(L"shaders/deferred_lighting_pass1.hlsl", nullptr, "VS", "vs_5_1");
  auto deferred_pass1_ps = d3dUtil::CompileShader(L"shaders/deferred_lighting_pass1.hlsl", nullptr, "PS", "ps_5_1");

  std::vector<D3D12_INPUT_ELEMENT_DESC> input_layout =
  {
    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
  { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
  { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
  };

  SamplerDesc default_sampler;
  pass1_signature_.Reset(4, 1);
  pass1_signature_.InitSampler(0, default_sampler, D3D12_SHADER_VISIBILITY_PIXEL);
  pass1_signature_[0].InitAsConstantBufferView(0, 0);
  pass1_signature_[1].InitAsConstantBufferView(1, 0);
  pass1_signature_[2].InitAsConstantBufferView(2, 0);
  pass1_signature_[3].InitAsDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_SHADER_VISIBILITY_PIXEL);
  pass1_signature_.Finalize();

  pass1_pso_.SetInputLayout(input_layout.data(), (UINT)input_layout.size());
  pass1_pso_.SetVertexShader(deferred_pass1_vs);
  pass1_pso_.SetPixelShader(deferred_pass1_ps);


  pass1_pso_.SetRootSignature(pass1_signature_);
  pass1_pso_.SetRasterizeState(CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT));
  pass1_pso_.SetBlendState(CD3DX12_BLEND_DESC(D3D12_DEFAULT));
  pass1_pso_.SetDepthStencilState(CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT));
  pass1_pso_.SetSampleMask(UINT_MAX);
  pass1_pso_.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
  pass1_pso_.SetRenderTargetFormat(Graphics::Core.MrtBufferFormat,
    Graphics::Core.DepthStencilFormat, (Graphics::Core.Msaa4xState() ? 4 : 1),
    (Graphics::Core.Msaa4xState() ? (Graphics::Core.Msaa4xQuanlity() - 1) : 0));
  pass1_pso_.Finalize();

  //  -----------------
  SamplerDesc depth_sampler;
  depth_sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
  depth_sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
  depth_sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
  depth_sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
  depth_sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
  pass2_signature_.Reset(4, 2);
  pass2_signature_.InitSampler(0, default_sampler, D3D12_SHADER_VISIBILITY_PIXEL);
  pass2_signature_.InitSampler(1, depth_sampler, D3D12_SHADER_VISIBILITY_PIXEL);
  pass2_signature_[0].InitAsConstantBufferView(0, 0);
  pass2_signature_[1].InitAsConstantBufferView(1, 0);
  pass2_signature_[2].InitAsConstantBufferView(2, 0);
  pass2_signature_[3].InitAsDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4, 0, 0, D3D12_SHADER_VISIBILITY_PIXEL);
  pass2_signature_.Finalize();

  auto deferred_pass2_vs = d3dUtil::CompileShader(L"shaders/deferred_lighting_pass2.hlsl", nullptr, "VS", "vs_5_1");
  auto deferred_pass2_ps = d3dUtil::CompileShader(L"shaders/deferred_lighting_pass2.hlsl", nullptr, "PS", "ps_5_1");

  pass2_pso_.SetInputLayout(input_layout.data(), (UINT)input_layout.size());

  pass2_pso_.SetVertexShader(deferred_pass2_vs);
  pass2_pso_.SetPixelShader(deferred_pass2_ps);

  pass2_pso_.SetRootSignature(pass2_signature_);
  pass2_pso_.SetRasterizeState(CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT));
  pass2_pso_.SetBlendState(CD3DX12_BLEND_DESC(D3D12_DEFAULT));
  //  pass2_pso_.SetDepthStencilState(CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT));
  pass2_pso_.SetSampleMask(UINT_MAX);
  pass2_pso_.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
  pass2_pso_.SetRenderTargetFormats(Graphics::Core.kMRTBufferCount, Graphics::Core.MrtFormats(),
    //  Graphics::Core.DepthStencilFormat, (Graphics::Core.Msaa4xState() ? 4 : 1),
    DXGI_FORMAT::DXGI_FORMAT_UNKNOWN, (Graphics::Core.Msaa4xState() ? 4 : 1),
    (Graphics::Core.Msaa4xState() ? (Graphics::Core.Msaa4xQuanlity() - 1) : 0));
  pass2_pso_.Finalize();

  ////  BuildQuan(-1.0f, 1.0f, 2.0f, 2.0f, 0.0f);
  ////  InitializeQuad();
  //Graphics::CreateQuad(-1.0f, 1.0f, 2.0f, 2.0f, 0.0f, vertex_buffer_, index_buffer_);
  //  ---------------------------
  auto deferred_pass3_vs = d3dUtil::CompileShader(L"shaders/deferred_lighting_pass3.hlsl", nullptr, "VS", "vs_5_1");
  auto deferred_pass3_ps = d3dUtil::CompileShader(L"shaders/deferred_lighting_pass3.hlsl", nullptr, "PS", "ps_5_1");

  //std::vector<D3D12_INPUT_ELEMENT_DESC> input_layout =
  //{
  //  { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
  //{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
  //{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
  //};

  //SamplerDesc default_sampler;
  pass3_signature_.Reset(5, 1);
  pass3_signature_.InitSampler(0, default_sampler, D3D12_SHADER_VISIBILITY_PIXEL);
  pass3_signature_[0].InitAsConstantBufferView(0, 0);
  pass3_signature_[1].InitAsConstantBufferView(1, 0);
  pass3_signature_[2].InitAsConstantBufferView(2, 0);
  pass3_signature_[3].InitAsDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 0, 0, D3D12_SHADER_VISIBILITY_PIXEL);
  pass3_signature_[4].InitAsDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 0, 1, D3D12_SHADER_VISIBILITY_PIXEL);
  pass3_signature_.Finalize();

  pass3_pso_.SetInputLayout(input_layout.data(), (UINT)input_layout.size());
  pass3_pso_.SetVertexShader(deferred_pass3_vs);
  pass3_pso_.SetPixelShader(deferred_pass3_ps);

  pass3_pso_.SetRootSignature(pass3_signature_);
  pass3_pso_.SetRasterizeState(CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT));
  pass3_pso_.SetBlendState(CD3DX12_BLEND_DESC(D3D12_DEFAULT));
  pass3_pso_.SetDepthStencilState(CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT));
  pass3_pso_.SetSampleMask(UINT_MAX);
  pass3_pso_.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
  pass3_pso_.SetRenderTargetFormat(Graphics::Core.BackBufferFormat,
    Graphics::Core.DepthStencilFormat, (Graphics::Core.Msaa4xState() ? 4 : 1),
    (Graphics::Core.Msaa4xState() ? (Graphics::Core.Msaa4xQuanlity() - 1) : 0));
  pass3_pso_.Finalize();

// ---------------------
  auto color_vs = d3dUtil::CompileShader(L"shaders/color_standard.hlsl", nullptr, "VS", "vs_5_1");
  auto color_ps = d3dUtil::CompileShader(L"shaders/color_standard.hlsl", nullptr, "PS", "ps_5_1");

  //std::vector<D3D12_INPUT_ELEMENT_DESC> input_layout =
  //{
  //  { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
  //{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
  //{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
  //};


  pass4_signature_.Reset(6, 1);
  pass4_signature_.InitSampler(0, default_sampler, D3D12_SHADER_VISIBILITY_PIXEL);
  pass4_signature_[0].InitAsConstantBufferView(0, 0);
  pass4_signature_[1].InitAsConstantBufferView(1, 0);
  pass4_signature_[2].InitAsConstantBufferView(2, 0);
  pass4_signature_[3].InitAsDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 0, 0, D3D12_SHADER_VISIBILITY_PIXEL);
  pass4_signature_[4].InitAsDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 2, D3D12_SHADER_VISIBILITY_PIXEL);
  pass4_signature_[5].InitAsDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, 2, D3D12_SHADER_VISIBILITY_PIXEL);
  pass4_signature_.Finalize();

  pass4_pso_.SetInputLayout(input_layout.data(), (UINT)input_layout.size());
  pass4_pso_.SetVertexShader(color_vs);
  pass4_pso_.SetPixelShader(color_ps);

  pass4_pso_.SetRootSignature(pass4_signature_);
  pass4_pso_.SetRasterizeState(CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT));
  pass4_pso_.SetBlendState(BlendAdditional);  
  pass4_pso_.SetDepthStencilState(CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT));
  pass4_pso_.SetSampleMask(UINT_MAX);
  pass4_pso_.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
  pass4_pso_.SetRenderTargetFormat(Graphics::Core.BackBufferFormat, Graphics::Core.DepthStencilFormat,
    (Graphics::Core.Msaa4xState() ? 4 : 1), (Graphics::Core.Msaa4xState() ? (Graphics::Core.Msaa4xQuanlity() - 1) : 0));
  pass4_pso_.Finalize();

//  --------------------

  OnResize(Graphics::Core.Width(), Graphics::Core.Height());
}

void DeferredLighting::OnRender(GraphicsContext& context) {
  auto& display_plane = Graphics::Core.DisplayPlane();
  //  pass1 start
  //context.SetViewports(&Graphics::Core.ViewPort());
  //context.SetScissorRects(&Graphics::Core.ScissorRect());
  context.TransitionResource(normal_buffer_, D3D12_RESOURCE_STATE_RENDER_TARGET, true);
  context.ClearColor(normal_buffer_);


  //for (int i = 0; i < Graphics::Core.kMRTBufferCount; ++i) {
  //  context.TransitionResource(Graphics::Core.GetMrtBuffer(i), D3D12_RESOURCE_STATE_RENDER_TARGET, true);
  //  context.ClearColor(Graphics::Core.GetMrtBuffer(i));
  //}

  context.ClearDepthStencil(Graphics::Core.DepthBuffer());
  //context.SetRenderTargets(Graphics::Core.kMRTBufferCount,
  //  Graphics::Core.MrtRtvs(), Graphics::Core.DepthBuffer().DSV());

  context.SetRenderTargets(1, &normal_buffer_.Rtv(), Graphics::Core.DepthBuffer().DSV());

  context.SetPipelineState(pass1_pso_);
  context.SetRootSignature(pass1_signature_);

  auto begin = render_queue_->Begin();
  for (auto it = begin; it != render_queue_->End(); ++it) {
    if (it->first == Graphics::RenderGroupType::RenderGroupType::kTransparent)
      continue;

    auto group_begin = it->second->Begin();
    auto group_end = it->second->End();
    for (auto it_obj = group_begin; it_obj != group_end; ++it_obj) {
      (*it_obj)->Render(context);
    }
  }

  //  pass1 finish

  context.CopyBuffer(Graphics::Core.DeferredDepthBuffer(), Graphics::Core.DepthBuffer());
  context.TransitionResource(Graphics::Core.DepthBuffer(), D3D12_RESOURCE_STATE_DEPTH_WRITE);

  for (int i = 0; i < Graphics::Core.kMRTBufferCount; ++i) {
    context.TransitionResource(Graphics::Core.GetMrtBuffer(i), D3D12_RESOURCE_STATE_RENDER_TARGET, true);
    context.ClearColor(Graphics::Core.GetMrtBuffer(i));
  }

  //context.TransitionResource(display_plane, D3D12_RESOURCE_STATE_RENDER_TARGET, true);
  //context.ClearColor(display_plane);

  ////  pass2 start

  context.SetRenderTargets(Graphics::Core.kMRTBufferCount, Graphics::Core.MrtRtvs(), Graphics::Core.DepthBuffer().DSV());

  context.SetPipelineState(pass2_pso_);
  context.SetRootSignature(pass2_signature_);

  context.SetDynamicConstantBufferView(2, sizeof(Graphics::MainConstants), &Graphics::MainConstants);

  D3D12_CPU_DESCRIPTOR_HANDLE handles[] = { normal_buffer_.Srv(), 
      Graphics::Core.DeferredDepthBuffer().DepthSRV() };

  ////  D3D12_CPU_DESCRIPTOR_HANDLE handles[Graphics::Core.kMRTBufferCount];

  ////for (int i = 0; i < Graphics::Core.kMRTBufferCount - 1; ++i) {
  ////  handles[i] = Graphics::Core.GetMrtBuffer(i).Srv();
  ////}
  //context.TransitionResource(Graphics::Core.DeferredDepthBuffer(), D3D12_RESOURCE_STATE_GENERIC_READ);
  ////  handles[Graphics::Core.kMRTBufferCount - 1] = Graphics::Core.DeferredDepthBuffer().DepthSRV();

  context.SetDynamicDescriptors(3, 0, _countof(handles), handles);

  context.SetVertexBuffer(mesh_quad_.VertexBuffer().VertexBufferView());
  context.SetIndexBuffer(mesh_quad_.IndexBuffer().IndexBufferView());
  context.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  context.DrawIndexedInstanced(mesh_quad_.IndexBuffer().ElementCount(), 1, 0, 0, 0);

  ////  pass3 start

  //for (int i = 0; i < Graphics::Core.kMRTBufferCount; ++i) {
  //  context.TransitionResource(Graphics::Core.GetMrtBuffer(i), D3D12_RESOURCE_STATE_RENDER_TARGET, true);
  //  context.ClearColor(Graphics::Core.GetMrtBuffer(i));
  //}

  context.ClearDepthStencil(Graphics::Core.DepthBuffer());

  context.TransitionResource(display_plane, D3D12_RESOURCE_STATE_RENDER_TARGET, true);
  context.SetRenderTargets(1, &display_plane.Rtv(), Graphics::Core.DepthBuffer().DSV());
  //  context.ClearColor(display_plane);

  context.SetPipelineState(pass3_pso_);
  context.SetRootSignature(pass3_signature_);

  //context.SetDynamicConstantBufferView(2, sizeof(Graphics::MainConstants), &Graphics::MainConstants);

  const int pass2_screen_map_count = 2;
  D3D12_CPU_DESCRIPTOR_HANDLE handles2[pass2_screen_map_count];
  for (int i = 0; i < pass2_screen_map_count; ++i) {
    handles2[i] = Graphics::Core.GetMrtBuffer(i).Srv();
  }
  context.SetDynamicDescriptors(4, 0, _countof(handles2), handles2);

  begin = render_queue_->Begin();
  for (auto it = begin; it != render_queue_->End(); ++it) {
    if (it->first == Graphics::RenderGroupType::RenderGroupType::kTransparent)
      continue;

    auto group_begin = it->second->Begin();
    auto group_end = it->second->End();
    for (auto it_obj = group_begin; it_obj != group_end; ++it_obj) {
      (*it_obj)->Render(context);
    }
  }

  //  pass4 start

  context.SetPipelineState(pass4_pso_);
  context.SetRootSignature(pass4_signature_);
  RenderGroup* transparent_group = render_queue_->GetGroup(
      Graphics::RenderGroupType::RenderGroupType::kTransparent);

  auto transparent_end = transparent_group->End();
  for (auto it = transparent_group->Begin();
      it != transparent_end; ++it) {
    (*it)->Render(context);
  }
}

void DeferredLighting::OnResize(uint32_t width, uint32_t height) {
  normal_buffer_.Destroy();
  normal_buffer_.Create(L"Deferred Lighting Normal", width, height, 
      1, Graphics::Core.MrtBufferFormat);

  color_buffer_.Destroy();
  color_buffer_.Create(L"Deferred Lighting Color", width, height,
      1, Graphics::Core.MrtBufferFormat);
}


};