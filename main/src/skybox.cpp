#include "skybox.h"
#include "mesh_geometry.h"
#include "sampler_manager.h"
#include "graphics_core.h"
#include "graphics_common.h"

#include "game_setting.h"

namespace Graphics {

namespace SkyBox {

using namespace Geometry;

//ByteAddressBuffer index_buffer_;
//StructuredBuffer vertex_buffer_;

Texture* SkyBoxTexture;

Mesh* mesh_sphere_;

GraphicsPso pso_;
RootSignature signature_;

ObjectConstants constant_;
//  Texture* sky_box_texture_;
Material material_;


//void InitBuffers();
void InitPso();

void Initialize() {
  //  InitBuffers();
  mesh_sphere_ = MeshManager::Instance().CreateSphere(1.0f, 20, 20);

  InitPso();

  constant_.World = MathHelper::Identity4x4();
  string texture_name = GameCore::GameSetting::GetStringValue("SkyTexture");
  auto w_texture_name = AnsiToWString(texture_name);

  material_.MatTransform = MathHelper::Identity4x4();
  XMMATRIX matTransform = XMLoadFloat4x4(&material_.MatTransform);
  XMStoreFloat4x4(&material_.MatTransform, XMMatrixTranspose(matTransform));

  SkyBoxTexture = TextureManager::Instance().RequestTexture(w_texture_name,
    D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURECUBE);
}

void InitPso() {
  auto sky_vs = d3dUtil::CompileShader(L"shaders/skybox.hlsl", nullptr, "VS", "vs_5_1");
  auto sky_ps = d3dUtil::CompileShader(L"shaders/skybox.hlsl", nullptr, "PS", "ps_5_1");

  std::vector<D3D12_INPUT_ELEMENT_DESC> input_layout =
  {
    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
  };

  SamplerDesc linear_wrap;
  //  linear_wrap.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
  signature_.Reset(4, 1);
  signature_.InitSampler(0, linear_wrap, D3D12_SHADER_VISIBILITY_PIXEL);
  signature_[0].InitAsConstantBufferView(0, 0);
  signature_[1].InitAsConstantBufferView(1, 0);
  signature_[2].InitAsConstantBufferView(2, 0);
  signature_[3].InitAsDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 2, D3D12_SHADER_VISIBILITY_PIXEL);

  signature_.Finalize();

  pso_.SetInputLayout(input_layout.data(), (UINT)input_layout.size());
  pso_.SetVertexShader(sky_vs);
  pso_.SetPixelShader(sky_ps);

  pso_.SetRootSignature(signature_);
  auto rasterazater = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
  rasterazater.CullMode = D3D12_CULL_MODE::D3D12_CULL_MODE_NONE;

  pso_.SetRasterizeState(rasterazater);
  pso_.SetBlendState(BlendAdditional);  // CD3DX12_BLEND_DESC(D3D12_DEFAULT)

  auto depth_stencil_state = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
  depth_stencil_state.DepthFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_LESS_EQUAL;
  pso_.SetDepthStencilState(depth_stencil_state);
  pso_.SetSampleMask(UINT_MAX);
  pso_.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
  pso_.SetRenderTargetFormat(Graphics::Core.BackBufferFormat, Graphics::Core.DepthStencilFormat,
    (Graphics::Core.Msaa4xState() ? 4 : 1), (Graphics::Core.Msaa4xState() ? (Graphics::Core.Msaa4xQuanlity() - 1) : 0));
  pso_.Finalize();
}


void Render(GraphicsContext& context, ColorBuffer& display_plane) {
  //GraphicsContext& context = GraphicsContext::Begin(L"Draw Context");
  //auto& display_plane = Graphics::Core.DisplayPlane();
  //context.SetViewports(&Graphics::Core.ViewPort());
  //context.SetScissorRects(&Graphics::Core.ScissorRect());
  context.TransitionResource(display_plane, D3D12_RESOURCE_STATE_RENDER_TARGET, true);

  context.ClearColor(display_plane);
  context.ClearDepthStencil(Graphics::Core.DepthBuffer());

  context.SetRenderTargets(1, &display_plane.Rtv(), Graphics::Core.DepthBuffer().DSV());
  context.SetRootSignature(signature_);
  context.SetPipelineState(pso_);

  context.SetDynamicConstantBufferView(0, sizeof(ObjectConstants), &constant_);
  //  context.SetDynamicConstantBufferView(0, sizeof(material_), &material_);
  context.SetDynamicConstantBufferView(2, sizeof(PassConstant), &Graphics::MainConstants);
  
  context.SetDynamicDescriptors(3, 0, 1, &SkyBoxTexture->SrvHandle());

  context.SetVertexBuffer(mesh_sphere_->VertexBuffer().VertexBufferView());
  context.SetIndexBuffer(mesh_sphere_->IndexBuffer().IndexBufferView());
  context.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  context.DrawIndexedInstanced(mesh_sphere_->IndexBuffer().ElementCount(), 1, 0, 0, 0);
}

}

}