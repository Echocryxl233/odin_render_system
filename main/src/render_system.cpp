#include <iostream>
#include <random>

#include "render_system.h"

#include "camera.h"
#include "command_context.h"
#include "defines_common.h"
#include "depth_of_field.h"
#include "graphics_core.h"
#include "game_setting.h"
#include "game_timer.h"
#include "model.h"
#include "math_helper.h"
#include "sampler_manager.h"


using namespace GameCore;


void RenderSystem::Initialize() {
  DebugUtility::Log(L"RenderSystem::Initialize()");
  
  car_.LoadFromFile("models/car_full.txt");
  skull_.LoadFromFile("models/skull_full.txt");

  render_object_.LoadFromFile("models/skull_full.txt");
  
  //  mesh_.LoadFromObj("meshes/puit_water_well.obj");

  BuildPso();
  BuildDeferredShadingPso();
  //  CreateTexture();

  BuildDebugPlane(-1.0f, 1.0f, 2.0f, 2.0f, 0.0f);
  BuildDebugPso();

  ssao_.Initialize(Graphics::Core.Width(), Graphics::Core.Height());

  //car_material_.DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
  //car_material_.FresnelR0 = XMFLOAT3(0.05f, 0.05f, 0.05f);
  //car_material_.Roughness = 0.3f;
  //car_material_.MatTransform = MathHelper::Identity4x4();
  //XMMATRIX matTransform = XMLoadFloat4x4(&car_material_.MatTransform);
  //XMStoreFloat4x4(&car_material_.MatTransform, XMMatrixTranspose(matTransform));

  ice_texture_2_ = TextureManager::Instance().RequestTexture(L"textures/ice.dds");
  grass_texture_2_ = TextureManager::Instance().RequestTexture(L"textures/grass.dds");
  InitializeLights();
  //float aspect_ratio = static_cast<float>(Graphics::Core.Width()) / Graphics::Core.Height();
  //XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f * MathHelper::Pi, aspect_ratio, 1.0f, 1000.0f);
  //XMStoreFloat4x4(&mProj, P);
  //for (int i=0; i<5; ++i) {
  //  RenderObject ro;
  //  ro.LoadFromFile("models/skull_full.txt");
  //  ro.Position().x = (float)(-9 + i*6);
  //  render_queue_.push_back(ro);
  //}
  InitializeRenderQueue();

  use_deferred = GameSetting::UseDeferred == 1;
  cout << "begin use_deferred : " << use_deferred << endl;
};

void RenderSystem::InitializeRenderQueue() {
  std::mt19937 random_engine(std::random_device{}());
  float object_area = GameSetting::ObjectArea;
  uniform_real_distribution<float> uniform_position(-object_area, object_area);
  uniform_real_distribution<float> uniform_height(0.0f, 1.0f);

  int object_count = GameSetting::ObjectCount;
  for (int i = 0; i < object_count; ++i) {
    RenderObject ro;
    ro.LoadFromFile("models/skull_full.txt");

    ro.Position().x = (float)uniform_position(random_engine);
    ro.Position().y = (float)uniform_height(random_engine);
    ro.Position().z = (float)uniform_position(random_engine);
    //auto format = DebugUtility::Format("render object index = %0, position = (%1, %2, %3)",
    //    to_string(i), to_string(ro.Position().x),
    //    to_string(ro.Position().y), to_string(ro.Position().z));
    //cout << format << endl;
    render_queue_.push_back(ro);
  }
  cout << endl;
}

void RenderSystem::InitializeLights() {
  //  XMVECTOR lightDir = -MathHelper::SphericalToCartesian(1.0f, mSunTheta, mSunPhi);

  //  XMStoreFloat3(&pass_constant_.Lights[0].Direction, lightDir);

  std::mt19937 random_engine(std::random_device{}());
  default_random_engine e;

  float light_area = GameSetting::LightArea;
  float light_height = GameSetting::LightHeight;

  uniform_real_distribution<float> uniform_color(0.0f, 1.0f);
  uniform_real_distribution<float> uniform_position(-light_area, light_area);
  uniform_real_distribution<float> falloffs(0.5f, 8.0f);
  normal_distribution<float> uniform_height(light_height, 1.0f);

  for (int i = 0; i < kPointLightCount; ++i) {
    //  std::cout << u(e) << endl;
    auto& light = pass_constant_.Lights[i];
    light.Position = { uniform_position(random_engine), uniform_height(random_engine), uniform_position(random_engine) };
    light.Strength = { uniform_color(random_engine), uniform_color(random_engine), uniform_color(random_engine) };
    light.FalloffStart = 0.001f;
    light.FalloffEnd =  3.0f;  // falloffs(random_engine);
    //auto format = DebugUtility::Format("light index = %0, position = (%1, %2, %3)",
    //    to_string(i), to_string(light.Position.x), 
    //    to_string(light.Position.y), to_string(light.Position.z));
    //cout << format << endl;

    //format = DebugUtility::Format("light index = %0, strength = (%1, %2, %3)",
    //  to_string(i), to_string(light.Strength.x),
    //  to_string(light.Strength.y), to_string(light.Strength.z));
    //cout << format << endl;

  }
  cout << endl;
}

void RenderSystem::BuildPso() {
  auto color_vs = d3dUtil::CompileShader(L"shaders/color_standard.hlsl", nullptr, "VS", "vs_5_1");
  auto color_ps = d3dUtil::CompileShader(L"shaders/color_standard.hlsl", nullptr, "PS", "ps_5_1");

  std::vector<D3D12_INPUT_ELEMENT_DESC> input_layout =
  {
      { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
      { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
      { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
  };

  SamplerDesc default_sampler;

  root_signature_.Reset(5, 1);
  root_signature_.InitSampler(0, default_sampler, D3D12_SHADER_VISIBILITY_PIXEL);
  root_signature_[0].InitAsConstantBufferView(0, 0);
  root_signature_[1].InitAsConstantBufferView(1, 0);
  root_signature_[2].InitAsConstantBufferView(2, 0);
  root_signature_[3].InitAsDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 0, 0, D3D12_SHADER_VISIBILITY_PIXEL);
  root_signature_[4].InitAsDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 2, D3D12_SHADER_VISIBILITY_PIXEL);
  root_signature_.Finalize();

  graphics_pso_.SetInputLayout(input_layout.data(), (UINT)input_layout.size());
  graphics_pso_.SetVertexShader(reinterpret_cast<BYTE*>(color_vs->GetBufferPointer()),
    (UINT)color_vs->GetBufferSize());
  graphics_pso_.SetPixelShader(reinterpret_cast<BYTE*>(color_ps->GetBufferPointer()),
    (UINT)color_ps->GetBufferSize());

  graphics_pso_.SetRootSignature(root_signature_);
  graphics_pso_.SetRasterizeState(CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT));
  graphics_pso_.SetBlendState(CD3DX12_BLEND_DESC(D3D12_DEFAULT));
  graphics_pso_.SetDepthStencilState(CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT));
  graphics_pso_.SetSampleMask(UINT_MAX);
  graphics_pso_.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
  graphics_pso_.SetRenderTargetFormat(Graphics::Core.BackBufferFormat, Graphics::Core.DepthStencilFormat,
    (Graphics::Core.Msaa4xState() ? 4 : 1), (Graphics::Core.Msaa4xState() ? (Graphics::Core.Msaa4xQuanlity() - 1) : 0));
  graphics_pso_.Finalize();

}

void RenderSystem::BuildDeferredShadingPso() {
  auto deferred_pass1_vs = d3dUtil::CompileShader(L"shaders/deferred_pass1.hlsl", nullptr, "VS", "vs_5_1");
  auto deferred_pass1_ps = d3dUtil::CompileShader(L"shaders/deferred_pass1.hlsl", nullptr, "PS", "ps_5_1");

  std::vector<D3D12_INPUT_ELEMENT_DESC> input_layout =
  {
    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
  };

  SamplerDesc default_sampler;
  deferred_pass1_signature_.Reset(4, 1);
  deferred_pass1_signature_.InitSampler(0, default_sampler, D3D12_SHADER_VISIBILITY_PIXEL);
  deferred_pass1_signature_[0].InitAsConstantBufferView(0, 0);
  deferred_pass1_signature_[1].InitAsConstantBufferView(1, 0);
  deferred_pass1_signature_[2].InitAsConstantBufferView(2, 0);
  deferred_pass1_signature_[3].InitAsDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_SHADER_VISIBILITY_PIXEL);
  deferred_pass1_signature_.Finalize();

  deferred_pass1_pso_.SetInputLayout(input_layout.data(), (UINT)input_layout.size());
  //deferred_pass1_pso_.SetVertexShader(reinterpret_cast<BYTE*>(deferred_pass1_vs->GetBufferPointer()),
  //  (UINT)deferred_pass1_vs->GetBufferSize());
  //deferred_pass1_pso_.SetPixelShader(reinterpret_cast<BYTE*>(deferred_pass1_ps->GetBufferPointer()),
  //  (UINT)deferred_pass1_ps->GetBufferSize());
  deferred_pass1_pso_.SetVertexShader(deferred_pass1_vs);
  deferred_pass1_pso_.SetPixelShader(deferred_pass1_ps);
  

  deferred_pass1_pso_.SetRootSignature(deferred_pass1_signature_);
  deferred_pass1_pso_.SetRasterizeState(CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT));
  deferred_pass1_pso_.SetBlendState(CD3DX12_BLEND_DESC(D3D12_DEFAULT));
  deferred_pass1_pso_.SetDepthStencilState(CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT));
  deferred_pass1_pso_.SetSampleMask(UINT_MAX);
  deferred_pass1_pso_.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
  deferred_pass1_pso_.SetRenderTargetFormats(Graphics::Core.kMRTBufferCount, Graphics::Core.MrtFormats(), 
    Graphics::Core.DepthStencilFormat, (Graphics::Core.Msaa4xState() ? 4 : 1), 
    (Graphics::Core.Msaa4xState() ? (Graphics::Core.Msaa4xQuanlity() - 1) : 0));
  deferred_pass1_pso_.Finalize();

  //  -----------------
  SamplerDesc depth_sampler;
  depth_sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
  depth_sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
  depth_sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
  depth_sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
  depth_sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
  deferred_pass2_signature_.Reset(4, 2);
  deferred_pass2_signature_.InitSampler(0, default_sampler, D3D12_SHADER_VISIBILITY_PIXEL);
  deferred_pass2_signature_.InitSampler(1, depth_sampler, D3D12_SHADER_VISIBILITY_PIXEL);
  deferred_pass2_signature_[0].InitAsConstantBufferView(0, 0);
  deferred_pass2_signature_[1].InitAsConstantBufferView(1, 0);
  deferred_pass2_signature_[2].InitAsConstantBufferView(2, 0);
  deferred_pass2_signature_[3].InitAsDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4, 0, 0, D3D12_SHADER_VISIBILITY_PIXEL);
  deferred_pass2_signature_.Finalize();

  auto deferred_pass2_vs = d3dUtil::CompileShader(L"shaders/deferred_pass2.hlsl", nullptr, "VS", "vs_5_1");
  auto deferred_pass2_ps = d3dUtil::CompileShader(L"shaders/deferred_pass2.hlsl", nullptr, "PS", "ps_5_1");
  
  deferred_pass2_pso_.SetInputLayout(input_layout.data(), (UINT)input_layout.size());

  deferred_pass2_pso_.SetVertexShader(deferred_pass2_vs);
  deferred_pass2_pso_.SetPixelShader(deferred_pass2_ps);

  deferred_pass2_pso_.SetRootSignature(deferred_pass2_signature_);
  deferred_pass2_pso_.SetRasterizeState(CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT));
  deferred_pass2_pso_.SetBlendState(CD3DX12_BLEND_DESC(D3D12_DEFAULT));
  deferred_pass2_pso_.SetDepthStencilState(CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT));
  deferred_pass2_pso_.SetSampleMask(UINT_MAX);
  deferred_pass2_pso_.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
  deferred_pass2_pso_.SetRenderTargetFormats(Graphics::Core.kMRTBufferCount, Graphics::Core.MrtFormats(),
    Graphics::Core.DepthStencilFormat, (Graphics::Core.Msaa4xState() ? 4 : 1),
    //  DXGI_FORMAT::DXGI_FORMAT_UNKNOWN, (Graphics::Core.Msaa4xState() ? 4 : 1),
    (Graphics::Core.Msaa4xState() ? (Graphics::Core.Msaa4xQuanlity() - 1) : 0));
  deferred_pass2_pso_.Finalize();
  
}

void RenderSystem::Update() {
  auto camera_view = GameCore::MainCamera.View4x4f();
  auto camera_proj = GameCore::MainCamera.Proj4x4f();

  XMMATRIX view = XMLoadFloat4x4(&GameCore::MainCamera.View4x4f());  //  GameCore::MainCamera.View4x4f()
  XMMATRIX proj = XMLoadFloat4x4(&GameCore::MainCamera.Proj4x4f());    //  GameCore::MainCamera.Proj4x4f()
  XMMATRIX view_proj = XMMatrixMultiply(view, proj);
  XMMATRIX view_proj_tex = XMMatrixMultiply(view_proj, kTextureTransform);

  //  XMStoreFloat4x4(&objConstants.World, XMMatrixTranspose(world));

  for(auto& ro : render_queue_) {
    ro.Update(GameCore::MainTimer);
  }

  XMStoreFloat4x4(&pass_constant_.View, XMMatrixTranspose(view));
  XMStoreFloat4x4(&pass_constant_.Proj, XMMatrixTranspose(proj));

  XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
  XMMATRIX invProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);

  XMStoreFloat4x4(&pass_constant_.InvView, XMMatrixTranspose(invView));
  XMStoreFloat4x4(&pass_constant_.InvProj, XMMatrixTranspose(invProj));
  XMStoreFloat4x4(&pass_constant_.ViewProj, XMMatrixTranspose(view_proj));
  XMStoreFloat4x4(&pass_constant_.ViewProjTex, XMMatrixTranspose(view_proj_tex));
  
  pass_constant_.AmbientLight = { 0.5f, 0.0f, 0.5f, 0.1f };
  pass_constant_.EyePosition = eye_pos_;
  //XMVECTOR lightDir = -MathHelper::SphericalToCartesian(1.0f, mSunTheta, mSunPhi);

  //XMStoreFloat3(&pass_constant_.Lights[0].Direction, lightDir);
  //pass_constant_.Lights[0].Strength = { 1.0f, 1.0f, 1.0f };

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

void RenderSystem::RenderSingleObject(GraphicsContext& context, RenderObject& object) {
  context.SetDynamicConstantBufferView(0, sizeof(object.GetObjConst()), &object.GetObjConst());
  context.SetDynamicConstantBufferView(1, sizeof(object.GetModel().GetMaterial()), &object.GetModel().GetMaterial());
  context.SetDynamicConstantBufferView(2, sizeof(pass_constant_), &pass_constant_);

  context.SetVertexBuffer(object.GetModel().GetMesh()->VertexBuffer().VertexBufferView());
  context.SetIndexBuffer(object.GetModel().GetMesh()->IndexBuffer().IndexBufferView());

 
  context.SetDynamicDescriptors(3, 0, object.GetModel().TextureCount(), object.GetModel().TextureData()); 

  context.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

  context.DrawIndexedInstanced(object.GetModel().GetMesh()->IndexBuffer().ElementCount(), 1, 0, 0, 0);
}

void RenderSystem::RenderObjects(GraphicsContext& context) {
  for (auto& ro : render_queue_) {
    RenderSingleObject(context, ro);
  }
}

void RenderSystem::RenderScene() {
  auto& display_plane = Graphics::Core.DisplayPlane();

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

  if (!use_deferred) {
    ForwardRender(draw_context);
  }
  else {
    DeferredRender(draw_context);
  }

    //  draw_context.Flush(true);

  {
    //draw_context.SetViewports(&Graphics::Core.ViewPort());
    //draw_context.SetScissorRects(&Graphics::Core.ScissorRect());
    //draw_context.TransitionResource(display_plane, D3D12_RESOURCE_STATE_RENDER_TARGET, true);

    //draw_context.ClearColor(display_plane);
    //draw_context.ClearDepthStencil(Graphics::Core.DepthBuffer());

  //  draw_context.SetRenderTargets(1, &Graphics::Core.DisplayPlane().Rtv(), Graphics::Core.DepthBuffer().DSV());
  //  draw_context.SetRootSignature(debug_signature_);
  //  draw_context.SetPipelineState(debug_pso_);

  ////
  ////  D3D12_CPU_DESCRIPTOR_HANDLE handles2[] = { ssao_.AmbientMap().Srv() };
  ////  draw_context.SetDynamicDescriptors(0, 0, _countof(handles2), handles2);

  //  draw_context.SetVertexBuffer(debug_vertex_buffer_.VertexBufferView());
  //  draw_context.SetIndexBuffer(debug_index_buffer_.IndexBufferView());
  //  draw_context.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  //  draw_context.DrawIndexedInstanced(debug_index_buffer_.ElementCount(), 1, 0, 0, 0);

  //  draw_context.Flush(true);
  }

  //PostProcess::DoF.Render(display_plane, 5);
  ////  draw_context.CopyBuffer(display_plane, PostProcess::DoF.BlurBuffer());
  //draw_context.CopyBuffer(display_plane, PostProcess::DoF.DoFBuffer());
  draw_context.TransitionResource(display_plane, D3D12_RESOURCE_STATE_PRESENT, true);

  draw_context.Finish(true);
}

void RenderSystem::ForwardRender(GraphicsContext& context) {
  auto& display_plane = Graphics::Core.DisplayPlane();

  context.SetViewports(&Graphics::Core.ViewPort());
  context.SetScissorRects(&Graphics::Core.ScissorRect());
  context.TransitionResource(display_plane, D3D12_RESOURCE_STATE_RENDER_TARGET, true);

  context.ClearColor(display_plane);
  context.ClearDepthStencil(Graphics::Core.DepthBuffer());
  context.SetRenderTargets(1, &Graphics::Core.DisplayPlane().Rtv(), Graphics::Core.DepthBuffer().DSV());

  context.SetPipelineState(graphics_pso_);
  context.SetRootSignature(root_signature_);

  RenderObjects(context);
}

void RenderSystem::DeferredRender(GraphicsContext& context) {
  auto& display_plane = Graphics::Core.DisplayPlane();

  context.SetViewports(&Graphics::Core.ViewPort());
  context.SetScissorRects(&Graphics::Core.ScissorRect());

  for (int i=0; i< Graphics::Core.kMRTBufferCount; ++i) {
    context.TransitionResource(Graphics::Core.GetMrtBuffer(i), D3D12_RESOURCE_STATE_RENDER_TARGET, true);
    context.ClearColor(Graphics::Core.GetMrtBuffer(i));
  }

  context.ClearDepthStencil(Graphics::Core.DepthBuffer());
  context.SetRenderTargets(Graphics::Core.kMRTBufferCount, 
      Graphics::Core.MrtRtvs(), Graphics::Core.DepthBuffer().DSV());

  context.SetPipelineState(deferred_pass1_pso_);
  context.SetRootSignature(deferred_pass1_signature_);

  RenderObjects(context);

  context.CopyBuffer(Graphics::Core.DeferredDepthBuffer(), Graphics::Core.DepthBuffer());
  context.TransitionResource(Graphics::Core.DepthBuffer(), D3D12_RESOURCE_STATE_DEPTH_WRITE);

  context.TransitionResource(display_plane, D3D12_RESOURCE_STATE_RENDER_TARGET, true);
  context.ClearColor(display_plane);

  context.SetRenderTargets(1, &display_plane.Rtv(),
       Graphics::Core.DepthBuffer().DSV());

  context.SetPipelineState(deferred_pass2_pso_);
  context.SetRootSignature(deferred_pass2_signature_);

  context.SetDynamicConstantBufferView(2, sizeof(pass_constant_), &pass_constant_);
  
  D3D12_CPU_DESCRIPTOR_HANDLE handles2[Graphics::Core.kMRTBufferCount];

  for (int i = 0; i < Graphics::Core.kMRTBufferCount-1; ++i) {
    handles2[i] = Graphics::Core.GetMrtBuffer(i).Srv();
  }
  context.TransitionResource(Graphics::Core.DeferredDepthBuffer(), D3D12_RESOURCE_STATE_GENERIC_READ);
  handles2[Graphics::Core.kMRTBufferCount-1] = Graphics::Core.DeferredDepthBuffer().DepthSRV();

  context.SetDynamicDescriptors(3, 0, _countof(handles2), handles2);

  context.SetVertexBuffer(debug_vertex_buffer_.VertexBufferView());
  context.SetIndexBuffer(debug_index_buffer_.IndexBufferView());
  context.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  context.DrawIndexedInstanced(debug_index_buffer_.ElementCount(), 1, 0, 0, 0);
}

void RenderSystem::BuildDebugPlane(float x, float y, float w, float h, float depth) {
  struct MeshVertex
  {
    MeshVertex() {}
    MeshVertex(
      const DirectX::XMFLOAT3& p,
      const DirectX::XMFLOAT3& n,
      const DirectX::XMFLOAT3& t,
      const DirectX::XMFLOAT2& uv) :
      Position(p),
      Normal(n),
      TangentU(t),
      TexC(uv) {}
    MeshVertex(
      float px, float py, float pz,
      float nx, float ny, float nz,
      float tx, float ty, float tz,
      float u, float v) :
      Position(px, py, pz),
      Normal(nx, ny, nz),
      TangentU(tx, ty, tz),
      TexC(u, v) {}

    DirectX::XMFLOAT3 Position;
    DirectX::XMFLOAT3 Normal;
    DirectX::XMFLOAT3 TangentU;
    DirectX::XMFLOAT2 TexC;
  };

  vector<MeshVertex> vertices_mesh(4);
  vector<uint32_t> indices32(6);

  // Position coordinates specified in NDC space.
  vertices_mesh[0] = MeshVertex(
    x, y - h, depth,
    0.0f, 0.0f, -1.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f);

  vertices_mesh[1] = MeshVertex(
    x, y, depth,
    0.0f, 0.0f, -1.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 0.0f);

  vertices_mesh[2] = MeshVertex(
    x + w, y, depth,
    0.0f, 0.0f, -1.0f,
    1.0f, 0.0f, 0.0f,
    1.0f, 0.0f);

  vertices_mesh[3] = MeshVertex(
    x + w, y - h, depth,
    0.0f, 0.0f, -1.0f,
    1.0f, 0.0f, 0.0f,
    1.0f, 1.0f);

  indices32[0] = 0;
  indices32[1] = 1;
  indices32[2] = 2;

  indices32[3] = 0;
  indices32[4] = 2;
  indices32[5] = 3;

  vector<Vertex> vertices(vertices_mesh.size());

  for (int i = 0; i < vertices_mesh.size(); ++i) {
    vertices[i].Pos = vertices_mesh[i].Position;
    vertices[i].Normal = vertices_mesh[i].Normal;
    vertices[i].TexCoord = vertices_mesh[i].TexC;
    //  vertices[i].TangentU = vertices_mesh[i].TangentU;
  }
  vector<uint16_t> indices(indices32.size());
  for (int i = 0; i < indices32.size(); ++i) {
    indices[i] = indices32[i];
  }

  debug_vertex_buffer_.Create(L"Debug Vertex Buffer", (uint32_t)vertices.size(), sizeof(Vertex), vertices.data());
  debug_index_buffer_.Create(L"Debug Index Buffer", (uint32_t)indices.size(), sizeof(uint16_t), indices.data());
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
  debug_pso_.SetDepthStencilState(CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT));
  debug_pso_.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
  debug_pso_.SetRenderTargetFormat(Graphics::Core.BackBufferFormat, Graphics::Core.DepthStencilFormat,
    (Graphics::Core.Msaa4xState() ? 4 : 1), (Graphics::Core.Msaa4xState() ? (Graphics::Core.Msaa4xQuanlity() - 1) : 0));
  debug_pso_.SetSampleMask(UINT_MAX);
  
  debug_pso_.Finalize();
  
}
