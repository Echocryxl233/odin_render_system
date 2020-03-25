#include "GI/shadow.h"
#include "graphics/command_context.h"
#include "graphics/graphics_core.h"
#include "graphics/graphics_common.h"
#include "GI/gi_utility.h"
#include "render_queue.h"
#include "game/camera.h"


namespace GI {
namespace Shadow {
ShadowMap MainShadow;
}
}

using namespace GI::Shadow;
using namespace GI::Utility;
using namespace Graphics;

ShadowMap::~ShadowMap() {
  depth_buffer_.Destroy();
  color_buffer_.Destroy();
}

void ShadowMap::Initialize(uint32_t width, uint32_t height) {
  width_ = width;
  height_ = height;

  viewport_ = { 0.0f, 0.0f, (float)width, (float)height, 0.0f, 1.0f };
  scissor_rect_ = { 0, 0, (int)width, (int)height };

  color_buffer_.Create(L"colorBuffer", width_, height_, 1, Graphics::Core.BackBufferFormat);
  depth_buffer_.Create(L"shadowBuffer", width_, height_, Graphics::Core.DepthStencilFormat);

  scene_bounds_.Center = XMFLOAT3(0.0f, 0.0f, 0.0f);
  scene_bounds_.Radius = sqrtf(65.0f * 65.0f + 65.0f * 65.0f);

  auto vs = d3dUtil::CompileShader(L"shaders/shadow.hlsl", nullptr, "VS", "vs_5_1");
  auto ps = d3dUtil::CompileShader(L"shaders/shadow.hlsl", nullptr, "PS", "ps_5_1");

  std::vector<D3D12_INPUT_ELEMENT_DESC> input_layout =
  {
    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    //  { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
  };

  signature_.Reset(2, 0);
  signature_[0].InitAsConstantBufferView(0);
  signature_[1].InitAsConstantBufferView(1);
  signature_.Finalize();

  pso_.SetInputLayout(input_layout.data(), (UINT)input_layout.size());
  pso_.SetVertexShader(vs);
  pso_.SetPixelShader(ps);
  pso_.SetRootSignature(signature_);
  pso_.SetRasterizeState(CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT));
  pso_.SetBlendState(BlendAdditional);  // CD3DX12_BLEND_DESC(D3D12_DEFAULT)
  pso_.SetDepthStencilState(CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT));
  pso_.SetSampleMask(UINT_MAX);
  pso_.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
  pso_.SetRenderTargetFormat(Graphics::Core.BackBufferFormat, Graphics::Core.DepthStencilFormat,
    (Graphics::Core.Msaa4xState() ? 4 : 1), (Graphics::Core.Msaa4xState() ? (Graphics::Core.Msaa4xQuanlity() - 1) : 0));
  pso_.Finalize();

}

void ShadowMap::Update() {

  XMVECTOR direction = XMLoadFloat3(&Graphics::MainConstants.Lights[0].Direction);
  XMVECTOR light_pos = -2.0f * direction * scene_bounds_.Radius;
  XMVECTOR target_pos = XMLoadFloat3(&scene_bounds_.Center);
  XMVECTOR look_up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
  light_view_ = XMMatrixLookAtLH(light_pos, target_pos, look_up);


  //  Light Space
  XMFLOAT3 sphere_center_LS;
  XMStoreFloat3(&sphere_center_LS, XMVector3TransformCoord(target_pos, light_view_));

  float l = sphere_center_LS.x - scene_bounds_.Radius;
  float r = sphere_center_LS.x + scene_bounds_.Radius;
  float t = sphere_center_LS.y + scene_bounds_.Radius;
  float b = sphere_center_LS.y - scene_bounds_.Radius;
  float n = sphere_center_LS.z - scene_bounds_.Radius;
  float f = sphere_center_LS.z + scene_bounds_.Radius;

  light_proj_ = XMMatrixOrthographicOffCenterLH(l, r, b, t, n, f);

  // Transform NDC space [-1,+1]^2 to texture space [0,1]^2
  XMMATRIX T(
    0.5f, 0.0f, 0.0f, 0.0f,
    0.0f, -0.5f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.5f, 0.5f, 0.0f, 1.0f);

  XMMATRIX s_transform =  light_view_ * light_proj_ * T ;
  XMStoreFloat4x4(&shadow_transform_, s_transform);
}

void ShadowMap::Render() {
  if (!is_enabled_)
    return;

  GraphicsContext& context = GraphicsContext::Begin(L"shadow context");

  context.SetViewports(&Graphics::Core.ViewPort());
  context.SetScissorRects(&Graphics::Core.ScissorRect());

  context.TransitionResource(color_buffer_, D3D12_RESOURCE_STATE_RENDER_TARGET, true);

  context.ClearColor(color_buffer_);
  context.ClearDepthStencil(depth_buffer_);
  context.SetRenderTargets(1, &color_buffer_.Rtv(), depth_buffer_.DSV());
  context.SetRootSignature(signature_);
  context.SetPipelineState(pso_);

  __declspec(align(16))
    struct PassConstant {
    XMFLOAT4X4 proj;
    XMFLOAT4X4 view_proj;
  } vs_constant;

  XMMATRIX view_proj_matrix = XMMatrixMultiply(light_view_, light_proj_);
  XMStoreFloat4x4(&vs_constant.proj, XMMatrixTranspose(light_proj_));
  XMStoreFloat4x4(&vs_constant.view_proj, XMMatrixTranspose(view_proj_matrix));

  context.SetDynamicConstantBufferView(1, sizeof(vs_constant), &vs_constant);

  vector<RenderObject*> objects;

  auto* opaque3 = Graphics::MainQueue.GetGroup(Graphics::RenderGroupType::kOpaque2);
  auto group_begin = opaque3->Begin();
  auto group_end = opaque3->End();

  for (auto it_obj = group_begin; it_obj != group_end; ++it_obj) {
    //  objects.push_back((*it_obj));

    auto render_object = *it_obj;
    auto& model = render_object->GetModel();
    auto& obj_constant = render_object->GetObjConst();
    context.SetDynamicConstantBufferView(0, sizeof(obj_constant), &obj_constant);
    context.SetVertexBuffer(model.GetMesh()->VertexBuffer().VertexBufferView());
    context.SetIndexBuffer(model.GetMesh()->IndexBuffer().IndexBufferView());

    context.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context.DrawIndexedInstanced(model.GetMesh()->IndexBuffer().ElementCount(), 1, 0, 0, 0);
  }


  context.Finish();
}

void ShadowMap::OnSetEnabled(bool enable) {

  if (!enable) {
    GraphicsContext& context = GraphicsContext::Begin(L"shadow clearer");
    context.ClearDepth(depth_buffer_);
    context.Finish();
  }
}
