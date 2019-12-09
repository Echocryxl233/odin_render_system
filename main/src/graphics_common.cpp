#include "camera.h"
#include "graphics_common.h"
#include "game_setting.h"
#include "render_queue.h"
#include "mesh_geometry.h"



namespace Graphics {
  CD3DX12_DEPTH_STENCIL_DESC DepthStateDisabled;
  PassConstant MainConstants;
};

namespace Graphics {

using namespace GameCore;
using namespace Geometry;

const XMMATRIX kTextureTransform = {
  0.5f, 0.0f, 0.0f, 0.0f,
  0.0f, -0.5f, 0.0f, 0.0f,
  0.0f, 0.0f, 1.0f, 0.0f,
  0.5f, 0.5f, 0.0f, 1.0f,
};

void InitializeCommonState() {
  DepthStateDisabled = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
  DepthStateDisabled.DepthEnable = false;
  DepthStateDisabled.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
}

void InitializeLights() {
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

void InitRenderQueue() {
  std::mt19937 random_engine(std::random_device{}());
  float object_area = GameSetting::ObjectArea;
  uniform_real_distribution<float> uniform_position(-object_area, object_area);
  uniform_real_distribution<float> uniform_height(0.0f, 1.0f);

  int object_count = GameSetting::ObjectCount;
  for (int i = 0; i < object_count; ++i) {
    RenderObject* ro = new RenderObject();
    ro->LoadFromFile("models/skull_full.txt");

    ro->Position().x = (float)uniform_position(random_engine);
    ro->Position().y = (float)uniform_height(random_engine);
    ro->Position().z = (float)uniform_position(random_engine);
    //auto format = DebugUtility::Format("render object index = %0, position = (%1, %2, %3)",
    //    to_string(i), to_string(ro.Position().x),
    //    to_string(ro.Position().y), to_string(ro.Position().z));
    //cout << format << endl;
    Graphics::MainRenderQueue.AddObject(ro);
  }
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

  MainConstants.AmbientLight = { 0.5f, 0.0f, 0.5f, 0.1f };
  MainConstants.EyePosition = MainCamera.Position();
  MainConstants.ZNear = MainCamera.ZNear();
  MainConstants.ZFar = MainCamera.ZFar();
}

void CreateQuad(float x, float y, float w, float h, float depth,
    GpuBuffer& vertex_buffer, GpuBuffer& index_buffer) {

  auto mesh_data = GeometryGenerator::Instance().CreateQuad(x, y, w, h, depth);
  vector<Vertex> vertices(mesh_data.Vertices.size());
  for (int i = 0; i < mesh_data.Vertices.size(); ++i) {
    Vertex& vertex = vertices[i];
    vertex.Position = mesh_data.Vertices[i].Position;
    vertex.Normal = mesh_data.Vertices[i].Normal;
    vertex.TexCoord = mesh_data.Vertices[i].TexC;
  }

  std::vector<std::uint16_t> indices = mesh_data.GetIndices16();

  vertex_buffer.Create(L"Vertex Buffer", (uint32_t)vertices.size(), sizeof(Vertex), vertices.data());
  index_buffer.Create(L"Index Buffer", (uint32_t)indices.size(), sizeof(uint16_t), indices.data());
}

}