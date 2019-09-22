#include "render_system.h"
#include "model.h"
#include "math_helper.h"
#include "graphics_core.h"
#include "command_context.h"
#include "defines_common.h"
#include "sampler_manager.h"

void RenderSystem::Initialize() {
  DebugUtility::Log(L"RenderSystem::Initialize()");
  
  LoadModel();
  BuildPso();
  //  CreateTexture();

  BuildDebugPlane(0.0f, 0.0f, 1.0f, 1.0f, 0.0f);
  BuildDebugPso();

  ssao_.Initialize(Graphics::Core.Width(), Graphics::Core.Height());

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

void RenderSystem::LoadModel() {
  std::ifstream fin("models/skull.txt");

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
  vertex_buffer_.Create(L"Vertex Buffer", 
    (uint32_t)vertices.size(), sizeof(Vertex), vertices.data());
  index_buffer_.Create(L"Index Buffer", 
    (uint32_t)indices.size(), sizeof(uint16_t), indices.data());

  car_texture_2_ = TextureManager::Instance().RequestTexture(L"textures/ice.dds");
  skull_texture_2_ = TextureManager::Instance().RequestTexture(L"textures/grass.dds");
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
  texTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 0);

  CD3DX12_DESCRIPTOR_RANGE ssao_tex_table;
  ssao_tex_table.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 2);

  SamplerDesc default_sampler;

  root_signature_.Reset(5, 1);
  root_signature_.InitSampler(0, default_sampler, D3D12_SHADER_VISIBILITY_PIXEL);
  root_signature_[0].InitAsConstantBufferView(0, 0);
  root_signature_[1].InitAsConstantBufferView(1, 0);
  root_signature_[2].InitAsConstantBufferView(2, 0);
  root_signature_[3].InitAsDescriptorTable(1, &texTable, D3D12_SHADER_VISIBILITY_PIXEL);
  root_signature_[4].InitAsDescriptorTable(1, &ssao_tex_table, D3D12_SHADER_VISIBILITY_PIXEL);
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
  XMMATRIX view_proj_tex = XMMatrixMultiply(view_proj, kTextureTransform);

  XMStoreFloat4x4(&objConstants.World, XMMatrixTranspose(world));

  XMStoreFloat4x4(&pass_constant_.View, XMMatrixTranspose(view));
  XMStoreFloat4x4(&pass_constant_.Proj, XMMatrixTranspose(proj));

  XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
  XMMATRIX invProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);

  XMStoreFloat4x4(&pass_constant_.InvView, XMMatrixTranspose(invView));
  XMStoreFloat4x4(&pass_constant_.InvProj, XMMatrixTranspose(invProj));
  XMStoreFloat4x4(&pass_constant_.ViewProj, XMMatrixTranspose(view_proj));
  XMStoreFloat4x4(&pass_constant_.ViewProjTex, XMMatrixTranspose(view_proj_tex));
  
  pass_constant_.AmbientLight = { 0.3f, 0.3f, 0.3f, 0.1f };
  pass_constant_.EyePosition = eye_pos_;
  XMVECTOR lightDir = -MathHelper::SphericalToCartesian(1.0f, mSunTheta, mSunPhi);

  XMStoreFloat3(&pass_constant_.Lights[0].Direction, lightDir);
  pass_constant_.Lights[0].Strength = { 1.0f, 1.0f, 1.0f };
}

void RenderSystem::RenderObjects() {
  //draw_context.SetDynamicConstantBufferView(0, sizeof(objConstants), &objConstants);
  //draw_context.SetDynamicConstantBufferView(1, sizeof(car_material_), &car_material_);
  //draw_context.SetDynamicConstantBufferView(2, sizeof(pass_constant_), &pass_constant_);
  //
  //draw_context.SetVertexBuffer(vertex_buffer_.VertexBufferView());
  //draw_context.SetIndexBuffer(index_buffer_.IndexBufferView());

  ////  D3D12_CPU_DESCRIPTOR_HANDLE handles[] = { car_texture_->DescriptorHandle }; car_texture_2_
  //D3D12_CPU_DESCRIPTOR_HANDLE handles[] = { car_texture_2_->SrvHandle() }; 
  //draw_context.SetDynamicDescriptors(3, 0, _countof(handles), handles);
  //
  //draw_context.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  //draw_context.DrawIndexedInstanced(index_buffer_.ElementCount(), 1, 0, 0, 0);
  //draw_context.Flush(true);
}

void RenderSystem::RenderScene() {
  auto& display_plane = Graphics::Core.DisplayPlane();

  GraphicsContext& draw_context = GraphicsContext::Begin(L"Draw Context");

  ssao_.BeginRender(draw_context, mView, mProj);
  {
    draw_context.SetDynamicConstantBufferView(0, sizeof(objConstants), &objConstants);
    draw_context.SetVertexBuffer(vertex_buffer_.VertexBufferView());
    draw_context.SetIndexBuffer(index_buffer_.IndexBufferView());
    draw_context.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    draw_context.DrawIndexedInstanced(index_buffer_.ElementCount(), 1, 0, 0, 0);
  }
  ssao_.EndRender(draw_context);
  draw_context.Flush(true);
  ssao_.ComputeAo(draw_context, mView, mProj);

  {
    draw_context.SetViewports(&Graphics::Core.ViewPort());
    draw_context.SetScissorRects(&Graphics::Core.ScissorRect());
    draw_context.TransitionResource(display_plane, D3D12_RESOURCE_STATE_RENDER_TARGET, true);

    draw_context.ClearColor(display_plane);
    draw_context.ClearDepthStencil(Graphics::Core.DepthBuffer());
    draw_context.SetRenderTargets(1, &Graphics::Core.DisplayPlane().Rtv(), Graphics::Core.DepthBuffer().DSV());

    draw_context.SetPipelineState(graphics_pso_);
    draw_context.SetRootSignature(root_signature_);

    draw_context.SetDynamicConstantBufferView(0, sizeof(objConstants), &objConstants);
    draw_context.SetDynamicConstantBufferView(1, sizeof(car_material_), &car_material_);
    draw_context.SetDynamicConstantBufferView(2, sizeof(pass_constant_), &pass_constant_);

    draw_context.SetVertexBuffer(vertex_buffer_.VertexBufferView());
    draw_context.SetIndexBuffer(index_buffer_.IndexBufferView());

    //  D3D12_CPU_DESCRIPTOR_HANDLE handles[] = { car_texture_->DescriptorHandle }; car_texture_2_
    D3D12_CPU_DESCRIPTOR_HANDLE handles[] = {  skull_texture_2_->SrvHandle() }; //  ssao_.AmbientMap().Srv()
    draw_context.SetDynamicDescriptors(3, 0, _countof(handles), handles);

    //  D3D12_CPU_DESCRIPTOR_HANDLE ssao_handle[] = {  ssao_.AmbientMap().Srv()  }; 
    D3D12_CPU_DESCRIPTOR_HANDLE ssao_handle[] = {  ssao_.AmbientMap().Srv()  }; 
    draw_context.SetDynamicDescriptors(4, 0, _countof(ssao_handle), ssao_handle);

    draw_context.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    draw_context.DrawIndexedInstanced(index_buffer_.ElementCount(), 1, 0, 0, 0);
    draw_context.Flush(true);
   }

  draw_context.SetViewports(&Graphics::Core.ViewPort());
  draw_context.SetScissorRects(&Graphics::Core.ScissorRect());
  draw_context.TransitionResource(display_plane, D3D12_RESOURCE_STATE_RENDER_TARGET, true);

  //draw_context.ClearColor(display_plane);
  //draw_context.ClearDepthStencil(Graphics::Core.DepthBuffer());

  draw_context.SetRenderTargets(1, &Graphics::Core.DisplayPlane().Rtv(), Graphics::Core.DepthBuffer().DSV());
  draw_context.SetRootSignature(debug_signature_);
  draw_context.SetPipelineState(debug_pso_);

  
  D3D12_CPU_DESCRIPTOR_HANDLE handles2[] = { ssao_.AmbientMap().Srv() };
  draw_context.SetDynamicDescriptors(0, 0, _countof(handles2), handles2);

  draw_context.SetVertexBuffer(debug_vertex_buffer_.VertexBufferView());
  draw_context.SetIndexBuffer(debug_index_buffer_.IndexBufferView());
  draw_context.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  draw_context.DrawIndexedInstanced(debug_index_buffer_.ElementCount(), 1, 0, 0, 0);
  
  draw_context.TransitionResource(display_plane, D3D12_RESOURCE_STATE_PRESENT, true);

  draw_context.Finish(true);
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
  CD3DX12_DESCRIPTOR_RANGE descriptor_range2;
  descriptor_range2.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 1);

  SamplerDesc debug_sampler;
  debug_signature_.Reset(1, 1);
  debug_signature_[0].InitAsDescriptorTable(1, &descriptor_range2, D3D12_SHADER_VISIBILITY_PIXEL);
  //  debug_signature_[1].InitAsConstantBufferView(1);
  //debug_signature_[1].InitAsConstants(1, 1);
  //debug_signature_[2].InitAsDescriptorTable(1, &descriptor_range1, D3D12_SHADER_VISIBILITY_PIXEL);
  //debug_signature_[3].InitAsDescriptorTable(1, &descriptor_range2, D3D12_SHADER_VISIBILITY_PIXEL);
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