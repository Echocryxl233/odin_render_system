#include "game/camera.h"
#include "game/game_setting.h"
#include "graphics/graphics_common.h"
#include "graphics/graphics_core.h"
#include "graphics/sampler_manager.h"
#include "render_queue.h"
#include "mesh_geometry.h"

#include "GI/shadow.h"


namespace Graphics {
  //  SamplerDesc SamplerShadowDesc;

  CD3DX12_DEPTH_STENCIL_DESC DepthStateDisabled;
  PassConstant MainConstants;

  D3D12_BLEND_DESC BlendAdditional;

  RenderQueue MainQueue;  
  ColorBuffer NormalMap;
}

namespace Graphics {

using namespace GameCore;
using namespace Geometry;

float light_theta_base_ = 0.0f;
float light_phi_base_ = 0.0f;
float light_theta_ = 0.0f;
float light_phi_ = 0.0f;

const DXGI_FORMAT kNormalMapFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;

const XMMATRIX kTextureTransform = {
  0.5f, 0.0f, 0.0f, 0.0f,
  0.0f, -0.5f, 0.0f, 0.0f,
  0.0f, 0.0f, 1.0f, 0.0f,
  0.5f, 0.5f, 0.0f, 1.0f,
};

void InitializeCommonState() {
  //SamplerShadowDesc.Filter = D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;


  DepthStateDisabled = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
  DepthStateDisabled.DepthEnable = false;
  DepthStateDisabled.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;

  D3D12_BLEND_DESC alpha_blend = {};
  alpha_blend.IndependentBlendEnable = false;
  alpha_blend.RenderTarget[0].BlendEnable = true;
  alpha_blend.RenderTarget[0].LogicOpEnable = false;
  alpha_blend.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
  alpha_blend.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
  alpha_blend.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
  alpha_blend.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
  alpha_blend.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
  alpha_blend.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
  alpha_blend.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_NOOP;
  alpha_blend.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
  BlendAdditional = alpha_blend;
}

void InitializeLights() {
    //  XMVECTOR lightDir = -MathHelper::SphericalToCartesian(1.0f, mSunTheta, mSunPhi);

    //  XMStoreFloat3(&pass_constant_.Lights[0].Direction, lightDir);

    std::mt19937 random_engine(std::random_device{}());
    default_random_engine e;

    float light_area = GameSetting::GetFloatValue("LightArea"); // GameSetting::LightArea;
    float light_height = GameSetting::GetFloatValue("LightHeight"); //  GameSetting::LightHeight;

    uniform_real_distribution<float> uniform_color(0.0f, 1.0f);
    uniform_real_distribution<float> uniform_position(-light_area, light_area);
    uniform_real_distribution<float> falloffs(0.5f, 8.0f);
    normal_distribution<float> uniform_height(light_height, 1.0f);

    auto& light = MainConstants.Lights[0];
    light.Strength = { 1.0f, 1.0f, 1.0f };
    light.Direction = { 0.57735f, -0.57735f, 0.57735f };
    MathHelper::CartesianToSpherical(light.Direction, light_theta_base_, light_phi_base_);
    ResetDirectLightRotation();

    for (int i = kDirectightCount; i < kPointLightCount; ++i) {
      //  std::cout << u(e) << endl;
      auto& light = MainConstants.Lights[i];
      light.Position = { uniform_position(random_engine), uniform_height(random_engine), uniform_position(random_engine) };
      light.Strength = { uniform_color(random_engine), uniform_color(random_engine), uniform_color(random_engine) };
      light.FalloffStart = 0.001f;
      light.FalloffEnd = falloffs(random_engine); //3.0f;  // 
      //auto format = DebugUtility::Format("light index = %0, position = (%1, %2, %3)",
      //    to_string(i), to_string(light.Position.x), 
      //    to_string(light.Position.y), to_string(light.Position.z));
      //cout << format << endl;

      //format = DebugUtility::Format("light index = %0, strength = (%1, %2, %3)",
      //  to_string(i), to_string(light.Strength.x),
      //  to_string(light.Strength.y), to_string(light.Strength.z));
      //cout << format << endl;
    }
}

void InitCommonBuffers() {
  NormalMap.Create(L"Common NormalMap", Core.Width(), Core.Height(), 1, kNormalMapFormat);
}

void ResizeBuffers(UINT width, UINT height) {
  NormalMap.Create(L"Common NormalMap", Core.Width(), Core.Height(), 1, kNormalMapFormat);
}

void InitRenderQueue() {
  std::mt19937 random_engine(std::random_device{}());
  float object_area = GameSetting::GetFloatValue("ObjectArea"); //  GameSetting::ObjectArea;
  uniform_real_distribution<float> uniform_position(-object_area, object_area);
  uniform_real_distribution<float> uniform_height(0.0f, 1.0f);

  int object_count = GameSetting::GetIntValue("ObjectCount");  //  GameSetting::ObjectCount / 2;
  RenderGroup* group1 = new RenderGroup(RenderGroupType::RenderGroupType::kOpaque);
  for (int i = 0; i < object_count; ++i) {
    RenderObject* ro = new RenderObject();
    ro->LoadFromFile("models/sphere_full.txt");

    ro->Position().x = (float)uniform_position(random_engine);
    ro->Position().y = (float)uniform_height(random_engine);
    ro->Position().z = (float)uniform_position(random_engine);
    //auto format = DebugUtility::Format("render object index = %0, position = (%1, %2, %3)",
    //    to_string(i), to_string(ro.Position().x),
    //    to_string(ro.Position().y), to_string(ro.Position().z));
    //cout << format << endl;
    //Graphics::MainRenderQueue.AddObject(ro);
    group1->AddObject(ro);
  }
  MainQueue.AddGroup(group1->GetType(), group1);

  //  object_count = GameSetting::ObjectCount / 2;
  RenderGroup* group2 = new RenderGroup(RenderGroupType::RenderGroupType::kOpaque2);
  for (int i = 0; i < object_count; ++i) {
    RenderObject* ro = new RenderObject();
    ro->LoadFromFile("models/sphere_full.txt");

    ro->Position().x = (float)uniform_position(random_engine);
    ro->Position().y = (float)uniform_height(random_engine);
    ro->Position().z = (float)uniform_position(random_engine);
    //auto format = DebugUtility::Format("render object index = %0, position = (%1, %2, %3)",
    //    to_string(i), to_string(ro.Position().x),
    //    to_string(ro.Position().y), to_string(ro.Position().z));
    //cout << format << endl;
    //Graphics::MainRenderQueue.AddObject(ro);
    group2->AddObject(ro);
  }
  MainQueue.AddGroup(group2->GetType(), group2);

  RenderGroup* group3 = new RenderGroup(RenderGroupType::RenderGroupType::kTransparent);
  for (int i=0; i<10; ++i)
  {
    RenderObject* transparent = new RenderObject();
    transparent->LoadFromFile("models/car_full.txt");

    transparent->Position().x = -15.0f + (float)i * 4.5f;
    transparent->Position().y = 7.0f;
    transparent->Position().z = 0.0f;
    //auto format = DebugUtility::Format("render object index = %0, position = (%1, %2, %3)",
    //    to_string(i), to_string(ro.Position().x),
    //    to_string(ro.Position().y), to_string(ro.Position().z));
    //cout << format << endl;
    //  Graphics::TransparentQueue.AddObject(transparent);
    group3->AddObject(transparent);
  }
  MainQueue.AddGroup(group3->GetType(), group3);

  RenderGroup* group4 = new RenderGroup(RenderGroupType::RenderGroupType::kOpaque3);
  for (int i = 0; i < 1; ++i)
  {
    RenderObject* grid = new RenderObject();
    grid->LoadFromFile("models/grid_full.txt");

    grid->Position().x = 0.0f;
    grid->Position().y = -10.0f;
    grid->Position().z = 0.0f;
    //auto format = DebugUtility::Format("render object index = %0, position = (%1, %2, %3)",
    //    to_string(i), to_string(ro.Position().x),
    //    to_string(ro.Position().y), to_string(ro.Position().z));
    //cout << format << endl;
    //  Graphics::TransparentQueue.AddObject(transparent);
    group4->AddObject(grid);
  }
  MainQueue.AddGroup(group4->GetType(), group4);
}

void UpdateConstants() {
  auto camera_view = GameCore::MainCamera.View4x4f();
  auto camera_proj = GameCore::MainCamera.Proj4x4f();

  XMMATRIX view = XMLoadFloat4x4(&GameCore::MainCamera.View4x4f());  //  GameCore::MainCamera.View4x4f()
  XMMATRIX proj = XMLoadFloat4x4(&GameCore::MainCamera.Proj4x4f());    //  GameCore::MainCamera.Proj4x4f()
  XMMATRIX view_proj = XMMatrixMultiply(view, proj);
  XMMATRIX view_proj_tex = XMMatrixMultiply(view_proj, kTextureTransform);

  XMStoreFloat4x4(&MainConstants.View, XMMatrixTranspose(view));
  XMStoreFloat4x4(&MainConstants.Proj, XMMatrixTranspose(proj));

  XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
  XMMATRIX invProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);
  XMMATRIX inv_view_proj = XMMatrixInverse(&XMMatrixDeterminant(view_proj), view_proj);

  XMStoreFloat4x4(&MainConstants.InvView, XMMatrixTranspose(invView));
  XMStoreFloat4x4(&MainConstants.InvProj, XMMatrixTranspose(invProj));
  XMStoreFloat4x4(&MainConstants.ViewProj, XMMatrixTranspose(view_proj));
  XMStoreFloat4x4(&MainConstants.InvViewProj, XMMatrixTranspose(inv_view_proj));
  XMStoreFloat4x4(&MainConstants.ViewProjTex, XMMatrixTranspose(view_proj_tex));

  XMMATRIX shadow_transform = XMLoadFloat4x4(&GI::Shadow::MainShadow.ShadowTransform());
  XMStoreFloat4x4(&MainConstants.ShadowTransform, XMMatrixTranspose(shadow_transform));
  
  MainConstants.AmbientLight = { 1.0f, 1.0f, 1.0f, 0.5f };
  MainConstants.EyePosition = MainCamera.Position();
  MainConstants.ZNear = MainCamera.ZNear();
  MainConstants.ZFar = MainCamera.ZFar();

  //DebugUtility::Log(L"eye position = (%0, %1, %2)", to_wstring(MainConstants.EyePosition.x),
  //    to_wstring(MainConstants.EyePosition.y), to_wstring(MainConstants.EyePosition.z));
}

void RotateDirectLight(float dx, float dy) {
  light_theta_ += dx;
  light_phi_ -= dy;

  XMVECTOR light_dir = MathHelper::SphericalToCartesian(1.0f, light_theta_, light_phi_);

  auto& light = MainConstants.Lights[0];
  XMStoreFloat3(&light.Direction, light_dir);
}

void ResetDirectLightRotation() {
  light_theta_ = light_theta_base_;
  light_phi_ = light_phi_base_;
  RotateDirectLight(0.0f, 0.0f);
  //auto& light = MainConstants.Lights[0];
  //light.Direction = light_base_direction_;
}

}