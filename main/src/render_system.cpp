#include "render_system.h"
#include "model.h"
#include "math_helper.h"
#include "graphics_core.h"
#include "command_context.h"
#include "defines_common.h"
#include "sampler_manager.h"

void RenderSystem::Initialize() {
  DebugUtility::Log(L"RenderSystem::Initialize()");
  
  LoadMesh();
  BuildPso();
  CreateTexture();

  car_material_.DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
  car_material_.FresnelR0 = XMFLOAT3(0.05f, 0.05f, 0.05f);
  car_material_.Roughness = 0.3f;
  car_material_.MatTransform = MathHelper::Identity4x4();
  XMMATRIX matTransform = XMLoadFloat4x4(&car_material_.MatTransform);
  XMStoreFloat4x4(&car_material_.MatTransform, XMMatrixTranspose(matTransform));

  float aspect_ratio = static_cast<float>(Graphics::Core.Width()) / Graphics::Core.Height();
  XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f * MathHelper::Pi, aspect_ratio, 1.0f, 1000.0f);
  XMStoreFloat4x4(&mProj, P);
};

void RenderSystem::LoadMesh() {
  std::ifstream fin("models/car.txt");

  UINT vertex_count = 0;
  UINT index_count = 0;
  std::string ignore;

  if (!fin)
  {
    MessageBox(0, L"models/skull.txt not found.", 0, 0);
    return;
  }

  fin >> ignore >> vertex_count;
  fin >> ignore >> index_count;
  fin >> ignore >> ignore >> ignore >> ignore;

  vector<Vertex> vertices(vertex_count);
  vector<uint16_t> indices(index_count * 3);

  XMFLOAT3 v_min_f3 = XMFLOAT3(+MathHelper::Infinity, +MathHelper::Infinity, +MathHelper::Infinity);
  XMFLOAT3 v_max_f3 = XMFLOAT3(-MathHelper::Infinity, -MathHelper::Infinity, -MathHelper::Infinity);

  XMVECTOR v_min = XMLoadFloat3(&v_min_f3);
  XMVECTOR v_max = XMLoadFloat3(&v_max_f3);

  //  DirectX::XMFLOAT3 ignore_normal;

  for (UINT i = 0; i < vertex_count; ++i) {
    fin >> vertices[i].Pos.x >> vertices[i].Pos.y >> vertices[i].Pos.z;
    fin >> vertices[i].Normal.x >> vertices[i].Normal.y >> vertices[i].Normal.z;
    vertices[i].TexCoord = {0.0f, 0.0f};
    XMVECTOR pos = XMLoadFloat3(&vertices[i].Pos);

    XMFLOAT3 spherePos;
    XMStoreFloat3(&spherePos, XMVector3Normalize(pos));

    float theta = atan2f(spherePos.z, spherePos.x);

    // Put in [0, 2pi].
    if (theta < 0.0f)
      theta += XM_2PI;

    float phi = acosf(spherePos.y);

    float u = theta / (2.0f * XM_PI);
    float v = phi / XM_PI;

    //  vertices_geo[i].TexCoord = { 0.0f, 0.0f };
    vertices[i].TexCoord = { u, v };

    v_min = XMVectorMin(v_min, pos);  //  find the min point
    v_max = XMVectorMax(v_max, pos);  //  find the max point
  }

  //BoundingBox bounds;
  //XMStoreFloat3(&bounds.Center, 0.5f * (v_min + v_max));
  //XMStoreFloat3(&bounds.Extents, 0.5f * (v_max - v_min));

  //BoundingSphere bounds;

  //XMStoreFloat3(&bounds.Center, 0.5f * (v_min + v_max));
  //XMFLOAT3 d;
  //XMStoreFloat3(&d, 0.5f * (v_max - v_min ));
  //float ratio = 0.5f;
  //bounds.Radius = ratio * sqrtf(d.x * d.x + d.y * d.y + d.z * d.z);


  fin >> ignore >> ignore >> ignore;
  for (UINT i = 0; i < index_count; ++i) {
    fin >> indices[i * 3 + 0] >> indices[i * 3 + 1] >> indices[i * 3 + 2];
  }

  fin.close();
  vertex_buffer_.Create(L"Vertex Buffer", Graphics::Core.Device(),
    (uint32_t)vertices.size(), sizeof(Vertex), vertices.data());
  index_buffer_.Create(L"Index Buffer", Graphics::Core.Device(),
    (uint32_t)indices.size(), sizeof(uint16_t), indices.data());
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

  CD3DX12_DESCRIPTOR_RANGE texTable;
  texTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

  SamplerDesc default_sampler;

  root_signature_.Reset(4, 1);
  root_signature_.InitSampler(0, default_sampler, D3D12_SHADER_VISIBILITY_PIXEL);
  //  root_signature_[0].InitAsDescriptorTable(1, D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
  //  root_signature_[0].InitAsDescriptorTable(1, &cbvTable);
  root_signature_[0].InitAsConstantBufferView(0, 0);
  root_signature_[1].InitAsConstantBufferView(1, 0);
  root_signature_[2].InitAsConstantBufferView(2, 0);
  root_signature_[3].InitAsDescriptorTable(1, &texTable, D3D12_SHADER_VISIBILITY_PIXEL);
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
  graphics_pso_.Finalize(Graphics::Core.Device());

}

void RenderSystem::CreateTexture() {
  car_texture_ = make_unique<TempTexture>();
  car_texture_->Name = "DefaultDiffuseTex";
  car_texture_->Filename = L"textures/ice.dds";
  GraphicsContext& texture_context = GraphicsContext::Begin(L"Texture Context");
  ASSERT_SUCCESSED(DirectX::CreateDDSTextureFromFile12(Graphics::Core.Device(),
    texture_context.GetCommandList(), car_texture_->Filename.c_str(),
    car_texture_->Resource, car_texture_->UploadHeap));

  texture_context.Finish(true);

  car_texture_->DescriptorHandle = DescriptorAllocatorManager::Instance().Allocate(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

  D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
  srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  srvDesc.Format = car_texture_->Resource->GetDesc().Format;
  srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
  srvDesc.Texture2D.MostDetailedMip = 0;
  srvDesc.Texture2D.MipLevels = car_texture_->Resource->GetDesc().MipLevels;
  srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

  Graphics::Core.Device()->CreateShaderResourceView(car_texture_->Resource.Get(), &srvDesc, car_texture_->DescriptorHandle);
};

void RenderSystem::UpdateCamera() {
  eye_pos_.x = mRadius * sinf(mPhi) * cosf(mTheta);
  eye_pos_.z = mRadius * sinf(mPhi) * sinf(mTheta);
  eye_pos_.y = mRadius * cosf(mPhi);

  //DebugUtility::Log(L"eye pos = (%0, %1, %2)", to_wstring(eye_pos_.x), to_wstring(eye_pos_.y), to_wstring(eye_pos_.z));

  // Build the view matrix.
  XMVECTOR pos = XMVectorSet(eye_pos_.x, eye_pos_.y, eye_pos_.z, 1.0f);
  XMVECTOR target = XMVectorZero();
  XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

  XMMATRIX view = XMMatrixLookAtLH(pos, target, up);
  XMStoreFloat4x4(&mView, view);
};

void RenderSystem::Update() {
  UpdateCamera();

  XMMATRIX world = XMLoadFloat4x4(&mWorld);
  XMMATRIX view = XMLoadFloat4x4(&mView);
  XMMATRIX proj = XMLoadFloat4x4(&mProj);
  XMMATRIX view_proj = XMMatrixMultiply(view, proj);

  XMStoreFloat4x4(&objConstants.World, XMMatrixTranspose(world));

  XMStoreFloat4x4(&pass_constant_.View, XMMatrixTranspose(view));
  XMStoreFloat4x4(&pass_constant_.Proj, XMMatrixTranspose(proj));

  XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
  XMMATRIX invProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);

  XMStoreFloat4x4(&pass_constant_.InvView, XMMatrixTranspose(invView));
  XMStoreFloat4x4(&pass_constant_.InvProj, XMMatrixTranspose(invProj));
  XMStoreFloat4x4(&pass_constant_.ViewProj, XMMatrixTranspose(view_proj));
  
  pass_constant_.AmbientLight = { 0.4f, 0.3f, 0.0f, 0.0f };
  pass_constant_.EyePosition = eye_pos_;
  XMVECTOR lightDir = -MathHelper::SphericalToCartesian(1.0f, mSunTheta, mSunPhi);

  XMStoreFloat3(&pass_constant_.Lights[0].Direction, lightDir);
  pass_constant_.Lights[0].Strength = { 1.0f, 1.0f, 1.0f };
}

void RenderSystem::RenderScene() {
  auto& display_plane = Graphics::Core.DisplayPlane();

  GraphicsContext& draw_context = GraphicsContext::Begin(L"Draw Context");

  draw_context.SetViewports(&Graphics::Core.ViewPort());
  draw_context.SetScissorRects(&Graphics::Core.ScissorRect());
  draw_context.TransitionResource(display_plane, D3D12_RESOURCE_STATE_RENDER_TARGET, true);

  draw_context.ClearColor(display_plane);
  draw_context.ClearDepthStencil(Graphics::Core.DepthBuffer());
  draw_context.SetRenderTargets(1, &Graphics::Core.DisplayPlane().GetRTV(), Graphics::Core.DepthBuffer().DSV());

  draw_context.SetPipelineState(graphics_pso_);
  draw_context.SetRootSignature(root_signature_);

  draw_context.SetDynamicConstantBufferView(0, sizeof(objConstants), &objConstants);
  draw_context.SetDynamicConstantBufferView(1, sizeof(car_material_), &car_material_);
  draw_context.SetDynamicConstantBufferView(2, sizeof(pass_constant_), &pass_constant_);
  
  draw_context.SetVertexBuffer(vertex_buffer_.VertexBufferView());
  draw_context.SetIndexBuffer(index_buffer_.IndexBufferView());
  draw_context.SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

  D3D12_CPU_DESCRIPTOR_HANDLE handles[] = { car_texture_->DescriptorHandle };
  draw_context.SetDynamicDescriptors(3, 0, _countof(handles), handles);
  
  draw_context.DrawIndexedInstanced(index_buffer_.ElementCount(), 1, 0, 0, 0);
  draw_context.TransitionResource(display_plane, D3D12_RESOURCE_STATE_PRESENT, true);

  draw_context.Finish(true);
}

void RenderSystem::OnMouseDown(WPARAM btnState, int x, int y) {
  //  DebugUtility::Log(L"OnMouseDown");  
}
void RenderSystem::OnMouseUp(WPARAM btnState, int x, int y) {
  //  DebugUtility::Log(L"OnMouseUp");
  ReleaseCapture();
}
void RenderSystem::OnMouseMove(WPARAM btnState, int x, int y) {
  
  if ((btnState & MK_LBUTTON) != 0)
  {
    // Make each pixel correspond to a quarter of a degree.
    float dx = XMConvertToRadians(0.25f * static_cast<float>(x - mLastMousePos.x));
    float dy = XMConvertToRadians(0.25f * static_cast<float>(y - mLastMousePos.y));

    // Update angles based on input to orbit camera around box.
    mTheta += dx;
    mPhi += dy;

    // Restrict the angle mPhi.
    mPhi = MathHelper::Clamp(mPhi, 0.1f, MathHelper::Pi - 0.1f);
  }
  else if ((btnState & MK_RBUTTON) != 0)
  {
    // Make each pixel correspond to 0.005 unit in the scene.
    float dx = 0.005f * static_cast<float>(x - mLastMousePos.x);
    float dy = 0.005f * static_cast<float>(y - mLastMousePos.y);

    // Update the camera radius based on input.
    mRadius += dx - dy;

    // Restrict the radius.
    mRadius = MathHelper::Clamp(mRadius, 3.0f, 15.0f);
  }

  mLastMousePos.x = x;
  mLastMousePos.y = y;
}