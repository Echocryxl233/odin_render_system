#include "gi_utility.h"
#include "pipeline_state.h"
#include "graphics_core.h"
#include "graphics_common.h"
#include "camera.h"

namespace GI {

using namespace Graphics;

namespace Utility {

RootSignature draw_normal_root_signature_;
GraphicsPso   draw_normal_pso_;

void Initialize() {

  draw_normal_root_signature_.Reset(2, 0);
  draw_normal_root_signature_[0].InitAsConstantBufferView(0);
  draw_normal_root_signature_[1].InitAsConstantBufferView(1);
  draw_normal_root_signature_.Finalize();

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

  draw_normal_pso_.SetRootSignature(draw_normal_root_signature_);
  draw_normal_pso_.SetBlendState(CD3DX12_BLEND_DESC(D3D12_DEFAULT));
  draw_normal_pso_.SetRasterizeState(CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT));
  draw_normal_pso_.SetDepthStencilState(CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT));
  draw_normal_pso_.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
  draw_normal_pso_.SetSampleMask(UINT_MAX);
  draw_normal_pso_.SetRenderTargetFormat(GI::kNormalMapFormat, Graphics::Core.DepthStencilFormat);
  draw_normal_pso_.Finalize();

}

void DrawNormalAndDepthMap(GraphicsContext& context, ColorBuffer& normal_map,
    DepthStencilBuffer& depth_buffer, const vector<RenderObject*>& objects) {
  ColorBuffer* buffer = &normal_map;
  //  ColorBuffer* buffer = &normal_map_1;

  //  ColorBuffer* buffer = &ambient_map_0_;

  //context.SetViewports(&Graphics::Core.ViewPort());
  //context.SetScissorRects(&Graphics::Core.ScissorRect());
  
  context.SetViewports(&Graphics::Core.ViewPort());
  context.SetScissorRects(&Graphics::Core.ScissorRect());

  context.TransitionResource(*buffer, D3D12_RESOURCE_STATE_RENDER_TARGET, true);

  context.ClearColor(*buffer);
  context.ClearDepthStencil(depth_buffer);
  context.SetRenderTargets(1, &buffer->Rtv(), depth_buffer.DSV());
  context.SetRootSignature(draw_normal_root_signature_);
  context.SetPipelineState(draw_normal_pso_);

  __declspec(align(16))
    struct PassConstant {
    XMFLOAT4X4 proj;
    XMFLOAT4X4 view_proj;
  } vs_constant;

  XMFLOAT4X4& view = GameCore::MainCamera.View4x4f();
  XMFLOAT4X4& proj = GameCore::MainCamera.Proj4x4f();

  XMMATRIX view_matrix = XMLoadFloat4x4(&view);
  XMMATRIX proj_matrix = XMLoadFloat4x4(&proj);
  XMMATRIX view_proj_matrix = XMMatrixMultiply(view_matrix, proj_matrix);

  XMStoreFloat4x4(&vs_constant.proj, XMMatrixTranspose(proj_matrix));
  XMStoreFloat4x4(&vs_constant.view_proj, XMMatrixTranspose(view_proj_matrix));

  context.SetDynamicConstantBufferView(1, sizeof(vs_constant), &vs_constant);

  for (int i=0; i< objects.size(); ++i) {
    RenderObject* object = objects[i];
    auto& model = object->GetModel();
    auto& obj_constant = object->GetObjConst();
    context.SetDynamicConstantBufferView(0, sizeof(obj_constant), &obj_constant);
    context.SetVertexBuffer(model.GetMesh()->VertexBuffer().VertexBufferView());
    context.SetIndexBuffer(model.GetMesh()->IndexBuffer().IndexBufferView());

    context.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context.DrawIndexedInstanced(model.GetMesh()->IndexBuffer().ElementCount(), 1, 0, 0, 0);
  }
}

};

};