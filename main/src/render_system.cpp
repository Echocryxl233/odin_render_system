#include <iostream>
#include <random>

#include "render_system.h"

#include "defines_common.h"
#include "GI/ssao.h"
#include "GI/gi_utility.h"
#include "GI/shadow.h"
#include "GI/ssr.h"
#include "graphics/command_context.h"
#include "graphics/graphics_core.h"
#include "graphics/graphics_common.h"
#include "graphics/sampler_manager.h"
#include "game/camera.h"
#include "game/game_setting.h"
#include "game/game_timer.h"
#include "model.h"
#include "math_helper.h"

#include "skybox.h"

#include "postprocess/bloom.h"
#include "postprocess/depth_of_field.h"

using namespace GameCore;

void RenderSystem::Initialize() {
  GI::Utility::Initialize();

  DebugUtility::Log(L"RenderSystem::Initialize()");

  forward_shading_ = new Graphics::ForwardShading();
  forward_shading_->Initialize();
  forward_shading_->SetRenderQueue(&Graphics::MainQueue);

  deferred_shading_ = new Graphics::DeferredShading();
  deferred_shading_->Initialize();
  deferred_shading_->SetRenderQueue(&Graphics::MainQueue);

  deferred_lighting_ = new Graphics::DeferredLighting();
  deferred_lighting_->Initialize();
  deferred_lighting_->SetRenderQueue(&Graphics::MainQueue);


  optional_systems_.push_back(forward_shading_);
  optional_systems_.push_back(deferred_shading_);
  optional_systems_.push_back(deferred_lighting_);
  current_optional_system_ = optional_systems_[current_optional_system_index_];

  BuildDebugPso();
  
  debug_mesh_.CreateQuad(0.0f, 0.0f, 1.0f, 1.0f, 0.0f);

  int width = Graphics::Core.Width() / 2;
  int height = Graphics::Core.Height() / 2;

  screen_viewport_.TopLeftX = 0.5f;
  screen_viewport_.TopLeftY = 0.5f;
  screen_viewport_.Width = static_cast<float>(height);
  screen_viewport_.Height = static_cast<float>(height);
  screen_viewport_.MinDepth = 0.0f;
  screen_viewport_.MaxDepth = 1.0f;

  //long x = (long)(0.5f * width);
  //long y = (long)(0.5f * height);

  //scissor_rect_ = { x, y, width, height };
  //use_deferred = GameSetting::GetBoolValue("UseDeferred"); //  GameSetting::UseDeferred == 1;
  //cout << "begin use_deferred : " << use_deferred << endl;
};

void RenderSystem::Update() {

  timer += GameCore::MainTimer.DeltaTime();
  if (GetAsyncKeyState('1') & 0x8000) {
    if (timer>0.2f) {
      use_deferred = !use_deferred;
      timer = 0.0f;
      cout << "current render mode : " << 
          (use_deferred ? "deferred" : "forward") << endl;
    }
  }
}

void RenderSystem::Render() {
  GraphicsContext& draw_context = GraphicsContext::Begin(L"Draw Context");
  auto& display_plane = Graphics::Core.DisplayPlane();

  GI::AO::MainSsao.ComputeAo();
  GI::Shadow::MainShadow.Render();

  current_optional_system_->Render();

  PostProcess::BloomEffect->Render(display_plane);

  if (GameSetting::GetBoolValue("UseDoF")) {
    PostProcess::DoF.Render(display_plane);
  }

  GI::Specular::MainSSR.RenderReflection();

  DrawDebug(draw_context);
  draw_context.TransitionResource(display_plane, D3D12_RESOURCE_STATE_PRESENT, true);
  draw_context.Finish(true);
}

void RenderSystem::BuildDebugPso() {
  SamplerDesc debug_sampler;
  debug_sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
  debug_signature_.Reset(1, 1);
  debug_signature_[0].InitAsDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 1, D3D12_SHADER_VISIBILITY_PIXEL);
  debug_signature_.InitSampler(0, debug_sampler, D3D12_SHADER_VISIBILITY_PIXEL);

  debug_signature_.Finalize();

  std::vector<D3D12_INPUT_ELEMENT_DESC> input_layout =
  {
    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
  };

  auto ssao_debug_vs = d3dUtil::CompileShader(L"shaders/debug.hlsl", nullptr, "VS", "vs_5_1");
  auto ssao_debug_ps = d3dUtil::CompileShader(L"shaders/debug.hlsl", nullptr, "PS", "ps_5_1");
  debug_pso_.SetInputLayout(input_layout.data(), (UINT)input_layout.size());
  debug_pso_.SetVertexShader(reinterpret_cast<BYTE*>(ssao_debug_vs->GetBufferPointer()),
    (UINT)ssao_debug_vs->GetBufferSize());
  debug_pso_.SetPixelShader(reinterpret_cast<BYTE*>(ssao_debug_ps->GetBufferPointer()),
    (UINT)ssao_debug_ps->GetBufferSize());

  debug_pso_.SetRootSignature(debug_signature_);
  debug_pso_.SetBlendState(CD3DX12_BLEND_DESC(D3D12_DEFAULT));
  debug_pso_.SetRasterizeState(CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT));
  debug_pso_.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
  debug_pso_.SetRenderTargetFormat(Graphics::Core.BackBufferFormat, DXGI_FORMAT_UNKNOWN,
    (Graphics::Core.Msaa4xState() ? 4 : 1), (Graphics::Core.Msaa4xState() ? (Graphics::Core.Msaa4xQuanlity() - 1) : 0));
  debug_pso_.SetSampleMask(UINT_MAX);
  
  debug_pso_.Finalize();
}

void RenderSystem::DrawDebug(GraphicsContext& context) {
  GraphicsContext& draw_context = GraphicsContext::Begin(L"Draw Context");

  auto& display_plane = Graphics::Core.DisplayPlane();

  draw_context.SetViewports(&Graphics::Core.ViewPort());
  draw_context.SetScissorRects(&Graphics::Core.ScissorRect());
  draw_context.TransitionResource(display_plane, D3D12_RESOURCE_STATE_RENDER_TARGET, true);
  
  draw_context.SetRenderTarget(&Graphics::Core.DisplayPlane().Rtv());
  draw_context.SetRootSignature(debug_signature_);
  draw_context.SetPipelineState(debug_pso_);

  //  GI::AO::MainSsao.AmbientMap().Srv()
//  GI::Specular::MainSSR.ColorMap().Srv()  //  GI::AO::MainSsao.DepthMap().DepthSRV()
  D3D12_CPU_DESCRIPTOR_HANDLE handles[] = { GI::Shadow::MainShadow.DepthBuffer().DepthSRV() };
  draw_context.SetDynamicDescriptors(0, 0, 1, handles );

  draw_context.SetVertexBuffer(debug_mesh_.VertexBuffer().VertexBufferView());
  draw_context.SetIndexBuffer(debug_mesh_.IndexBuffer().IndexBufferView());
  draw_context.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  draw_context.DrawIndexedInstanced(debug_mesh_.IndexBuffer().ElementCount(), 1, 0, 0, 0);
  //context.Flush();
  draw_context.Finish();
}

void RenderSystem::PrevOptionalSystem() {
  const float delay = 0.2f;  
  auto total = GameCore::MainTimer.TotalTime();
  if (total - select_timer_ < delay)
    return;

  select_timer_ = GameCore::MainTimer.TotalTime();
  size_t count = optional_systems_.size();
  current_optional_system_index_ = (current_optional_system_index_ + count - 1) % count;
  current_optional_system_ = optional_systems_[current_optional_system_index_];

}

void RenderSystem::NextOptionalSystem() {
  const float delay = 0.2f;
  auto total = GameCore::MainTimer.TotalTime();
  if (total - select_timer_ < delay)
    return;

  select_timer_ = GameCore::MainTimer.TotalTime();
  size_t count = optional_systems_.size();
  current_optional_system_index_ = (current_optional_system_index_ + 1) % count;
  current_optional_system_ = optional_systems_[current_optional_system_index_];
}

