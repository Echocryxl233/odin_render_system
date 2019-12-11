#include <iostream>
#include <random>

#include "render_system.h"

#include "camera.h"
#include "command_context.h"
#include "defines_common.h"
#include "depth_of_field.h"
#include "graphics_core.h"
#include "graphics_common.h"
#include "game_setting.h"
#include "game_timer.h"
#include "model.h"
#include "math_helper.h"
#include "sampler_manager.h"


using namespace GameCore;

void RenderSystem::Initialize() {
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

  optional_system_ = deferred_lighting_;

  BuildDebugPso();
  Graphics::CreateQuad(0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 
      debug_vertex_buffer_, debug_index_buffer_);
  ssao_.Initialize(Graphics::Core.Width(), Graphics::Core.Height());

  use_deferred = GameSetting::UseDeferred == 1;
  cout << "begin use_deferred : " << use_deferred << endl;
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

void RenderSystem::RenderScene() {
  GraphicsContext& draw_context = GraphicsContext::Begin(L"Draw Context");

  if (false) {
    //ssao_.BeginRender(draw_context, MainCamera.View4x4f(), MainCamera.Proj4x4f());
    //{
    //  draw_context.SetDynamicConstantBufferView(0, sizeof(objConstants), &objConstants);
    //  draw_context.SetVertexBuffer(car_.GetMesh()->VertexBuffer().VertexBufferView());
    //  draw_context.SetIndexBuffer(car_.GetMesh()->IndexBuffer().IndexBufferView());
    //  draw_context.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    //  draw_context.DrawIndexedInstanced(car_.GetMesh()->IndexBuffer().ElementCount(), 1, 0, 0, 0);
    //}
    //ssao_.EndRender(draw_context);
    //draw_context.Flush(true);
    //ssao_.ComputeAo(draw_context, MainCamera.View4x4f(), MainCamera.Proj4x4f());
  }

  optional_system_->Render(draw_context);

  auto& display_plane = Graphics::Core.DisplayPlane();
 
  //PostProcess::DoF.Render(display_plane, 5);
  //draw_context.CopyBuffer(display_plane, PostProcess::DoF.DoFBuffer());
  DrawDebug(draw_context);
  draw_context.TransitionResource(display_plane, D3D12_RESOURCE_STATE_PRESENT, true);

  draw_context.Finish(true);
}

void RenderSystem::BuildDebugPso() {
  SamplerDesc debug_sampler;
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
  auto& display_plane = Graphics::Core.DisplayPlane();

  context.SetViewports(&Graphics::Core.ViewPort());
  context.SetScissorRects(&Graphics::Core.ScissorRect());
  context.TransitionResource(display_plane, D3D12_RESOURCE_STATE_RENDER_TARGET, true);
  
  context.SetRenderTarget(&Graphics::Core.DisplayPlane().Rtv());
  context.SetRootSignature(debug_signature_);
  context.SetPipelineState(debug_pso_);

  D3D12_CPU_DESCRIPTOR_HANDLE handles2[] = { Graphics::Core.GetMrtBuffer(1).Srv() };
  context.SetDynamicDescriptors(0, 0, _countof(handles2), handles2);

  context.SetVertexBuffer(debug_vertex_buffer_.VertexBufferView());
  context.SetIndexBuffer(debug_index_buffer_.IndexBufferView());
  context.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  context.DrawIndexedInstanced(debug_index_buffer_.ElementCount(), 1, 0, 0, 0);
  context.Flush();
}
