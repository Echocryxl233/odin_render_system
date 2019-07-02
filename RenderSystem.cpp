#include "RenderSystem.h"
#include "GeometryGenerator.h"
#include <iostream> 

using namespace std;

namespace OdinRenderSystem {

RenderSystem::RenderSystem(HINSTANCE h_instance)
  :Application(h_instance)
{
}


RenderSystem::~RenderSystem()
{
}

void RenderSystem::Draw(const GameTimer& gt)
{
    auto cmdListAlloc = current_frame_resource_->CmdListAlloc;

    // Reuse the memory associated with command recording.
    // We can only reset when the associated command lists have finished execution on the GPU.
    ThrowIfFailed(cmdListAlloc->Reset());

    //  ThrowIfFailed(d3d_command_list_->Reset(cmdListAlloc.Get(), pipeline_state_object_.Get()));
    ThrowIfFailed(d3d_command_list_->Reset(cmdListAlloc.Get(), pipeline_state_objects_["Opaque"].Get()));

    d3d_command_list_->RSSetViewports(1, &screen_viewport_);
    d3d_command_list_->RSSetScissorRects(1, &scissor_rect_);

    // Indicate a state transition on the resource usage.
	  d3d_command_list_->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

    // Clear the back buffer and depth buffer.
    d3d_command_list_->ClearRenderTargetView(CurrentBackBufferView(), Colors::LightSteelBlue, 0, nullptr);
    d3d_command_list_->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

    // Specify the buffers we are going to render to.
    d3d_command_list_->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());

    ID3D12DescriptorHeap* descriptorHeaps[] = { srv_descriptor_heap_.Get() };
	  d3d_command_list_->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	  d3d_command_list_->SetGraphicsRootSignature(root_signature_.Get());

    UINT passCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(PassConstants));
    auto passCB = current_frame_resource_->PassCB->Resource();
	  d3d_command_list_->SetGraphicsRootConstantBufferView(1, passCB->GetGPUVirtualAddress());

    auto material_buffer = current_frame_resource_->MaterialBuffer->Resource();
	  d3d_command_list_->SetGraphicsRootShaderResourceView(2, material_buffer->GetGPUVirtualAddress());

    d3d_command_list_->SetGraphicsRootDescriptorTable(3, srv_descriptor_heap_->GetGPUDescriptorHandleForHeapStart());

    DrawRenderItems(d3d_command_list_.Get(), items_layers_[(int)RenderLayer::kOpaque]);

      d3d_command_list_->OMSetStencilRef(1);
      d3d_command_list_->SetPipelineState(pipeline_state_objects_[pso_name_mirror].Get());
      DrawRenderItems(d3d_command_list_.Get(), items_layers_[(int)RenderLayer::kMirrors]);

    //  d3d_command_list_->SetGraphicsRootConstantBufferView(3, passCB->GetGPUVirtualAddress() + 1*passCBByteSize);
    d3d_command_list_->SetPipelineState(pipeline_state_objects_[pso_name_reflection].Get());
    DrawRenderItems(d3d_command_list_.Get(), items_layers_[(int)RenderLayer::kReflected]);
    

      d3d_command_list_->SetGraphicsRootConstantBufferView(3, passCB->GetGPUVirtualAddress());
      d3d_command_list_->OMSetStencilRef(0);

      d3d_command_list_->SetPipelineState(pipeline_state_objects_["Transparent"].Get());
      DrawRenderItems(d3d_command_list_.Get(), items_layers_[(int)RenderLayer::kTransparent]);

    blur_filter_->Execute(d3d_command_list_.Get(), post_process_root_signature_.Get(), 
		  pipeline_state_objects_["horzBlur"].Get(), pipeline_state_objects_["vertBlur"].Get(), CurrentBackBuffer(), 4);

    d3d_command_list_->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		  D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_COPY_DEST));

	  d3d_command_list_->CopyResource(CurrentBackBuffer(), blur_filter_->Output());

    // Transition to PRESENT state.
	  d3d_command_list_->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		  D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PRESENT));

    // Indicate a state transition on the resource usage.
	 // d3d_command_list_->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		//D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

    // Done recording commands.
    ThrowIfFailed(d3d_command_list_->Close());

    // Add the command list to the queue for execution.
    ID3D12CommandList* cmdsLists[] = { d3d_command_list_.Get() };
    d3d_command_queue_->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

    // Swap the back and front buffers
    ThrowIfFailed(swap_chain_->Present(0, 0));
	  current_back_buffer_index_ = (current_back_buffer_index_ + 1) % kSwapBufferCount;

    // Advance the fence value to mark commands up to this fence point.
    current_frame_resource_->Fence = ++current_fence_;
    
    // Add an instruction to the command queue to set a new fence point. 
    // Because we are on the GPU timeline, the new fence point won't be 
    // set until the GPU finishes processing all the commands prior to this Signal().
    d3d_command_queue_->Signal(d3d_fence_.Get(), current_fence_);
}

void RenderSystem::DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems)
{
  UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));
  UINT matCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(MaterialConstants));
 
	auto objectCB = current_frame_resource_->ObjectCB->Resource();
  auto materialCB = current_frame_resource_->MaterialBuffer->Resource();

  // For each render item...
  for(size_t i = 0; i < ritems.size(); ++i)
  {
    auto ri = ritems[i];

    cmdList->IASetVertexBuffers(0, 1, &ri->Geo->VertexBufferView());
    cmdList->IASetIndexBuffer(&ri->Geo->IndexBufferView());
    cmdList->IASetPrimitiveTopology(ri->PrimitiveType);

    D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = objectCB->GetGPUVirtualAddress();
    objCBAddress += ri->ObjectCBIndex*objCBByteSize;

    cmdList->SetGraphicsRootConstantBufferView(0, objCBAddress);

    cmdList->DrawIndexedInstanced(ri->IndexCount, 1, ri->StartIndexLocation, ri->BaseVertexLocation, 0);
  }
}


void RenderSystem::Update(const GameTimer& gt) {
  OnKeyboardInput(gt);

  // Update the constant buffer with the latest worldViewProj matrix.
	//ObjectConstants objConstants;
  UpdateCamera(gt);

  frame_resource_index_ = (frame_resource_index_ + 1) % kNumFrameResources;
  current_frame_resource_ = frame_resources_[frame_resource_index_].get();
  if(current_frame_resource_->Fence != 0 &&  d3d_fence_->GetCompletedValue() < current_frame_resource_->Fence)
	{
		HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);
    ThrowIfFailed(d3d_fence_->SetEventOnCompletion(current_frame_resource_->Fence, eventHandle));
		WaitForSingleObject(eventHandle, INFINITE);
    CloseHandle(eventHandle);
	}

  //  AnimateMaterials(gt);
  UpdateObjectCBs(gt);
  UpdateMainPassCB(gt);
  UpdateMaterialBuffer(gt);
  UpdateReflectedPassCB(gt);
  //  UpdateWave(gt);
  
}

void RenderSystem::UpdateCamera(const GameTimer& gt) {
  // Convert Spherical to Cartesian coordinates.

  eye_position_.x = mRadius*sinf(mPhi)*cosf(mTheta);
  eye_position_.z = mRadius*sinf(mPhi)*sinf(mTheta);
  eye_position_.y = mRadius*cosf(mPhi);


  // Build the view matrix.
  XMVECTOR pos = XMVectorSet(eye_position_.x, eye_position_.y, eye_position_.z, 1.0f);
  XMVECTOR target = XMVectorZero();
  XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

  XMMATRIX view = XMMatrixLookAtLH(pos, target, up);
  XMStoreFloat4x4(&mView, view);
}

void RenderSystem::UpdateObjectCBs(const GameTimer& gt) {
  auto current_object_cb = current_frame_resource_->ObjectCB.get();
  
  for(auto& item : render_items_) {
    if (item->NumFrameDirty > 0) {
      XMMATRIX world = XMLoadFloat4x4(&item->World);
      XMMATRIX texcoord = XMLoadFloat4x4(&item->TexTransform);
      ObjectConstants object_CB;
      XMStoreFloat4x4(&object_CB.World, XMMatrixTranspose(world));  //  why is the transpose matrix of world
      XMStoreFloat4x4(&object_CB.TexTransform, XMMatrixTranspose(texcoord));  
      object_CB.MaterialIndex = item->Mat->MatCBIndex;

      current_object_cb->CopyData(item->ObjectCBIndex, object_CB);
      --item->NumFrameDirty;
    }
  }
}

void RenderSystem::UpdateMainPassCB(const GameTimer& gt) {
  //  XMMATRIX view = XMLoadFloat4x4(&mView);
	//  XMMATRIX proj = XMLoadFloat4x4(&mProj);

  XMMATRIX view = camera_.GetView();
	XMMATRIX proj = camera_.GetProj();

	XMMATRIX viewProj = XMMatrixMultiply(view, proj);
	XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
	XMMATRIX invProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);
	XMMATRIX invViewProj = XMMatrixInverse(&XMMatrixDeterminant(viewProj), viewProj);

	XMStoreFloat4x4(&main_pass_cb_.View, XMMatrixTranspose(view));
	XMStoreFloat4x4(&main_pass_cb_.InvView, XMMatrixTranspose(invView));
	XMStoreFloat4x4(&main_pass_cb_.Proj, XMMatrixTranspose(proj));
	XMStoreFloat4x4(&main_pass_cb_.InvProj, XMMatrixTranspose(invProj));
	XMStoreFloat4x4(&main_pass_cb_.ViewProj, XMMatrixTranspose(viewProj));
	XMStoreFloat4x4(&main_pass_cb_.InvViewProj, XMMatrixTranspose(invViewProj));
	//  main_pass_cb_.EyePosW = eye_position_;
  main_pass_cb_.EyePosW = camera_.GetPosition3f();
	main_pass_cb_.RenderTargetSize = XMFLOAT2((float)client_width_, (float)client_height_);
	main_pass_cb_.InvRenderTargetSize = XMFLOAT2(1.0f / client_width_, 1.0f / client_height_);
	main_pass_cb_.NearZ = 1.0f;
	main_pass_cb_.FarZ = 1000.0f;
	main_pass_cb_.TotalTime = gt.TotalTime();
	main_pass_cb_.DeltaTime = gt.DeltaTime();

  main_pass_cb_.AmbientLight = { 0.25f, 0.25f, 0.35f, 1.0f };

  XMVECTOR lightDir = -MathHelper::SphericalToCartesian(1.0f, mSunTheta, mSunPhi);
  XMStoreFloat3(&main_pass_cb_.Lights[0].Direction, lightDir);
  main_pass_cb_.Lights[0].Strength = { 1.0f, 1.0f, 0.9f };
  

  auto current_pass_cb = current_frame_resource_->PassCB.get();
  current_pass_cb->CopyData(0, main_pass_cb_);
}

void RenderSystem::UpdateReflectedPassCB(const GameTimer& gt)
{
	mReflectedPassCB = main_pass_cb_;

	XMVECTOR mirrorPlane = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f); // xy plane
	XMMATRIX R = XMMatrixReflect(mirrorPlane);

	// Reflect the lighting.
	/*for(int i = 0; i < 3; ++i)
	{
		XMVECTOR lightDir = XMLoadFloat3(&main_pass_cb_.Lights[i].Direction);
		XMVECTOR reflectedLightDir = XMVector3TransformNormal(lightDir, R);
		XMStoreFloat3(&mReflectedPassCB.Lights[i].Direction, reflectedLightDir);
	}*/

  XMVECTOR lightDir = XMLoadFloat3(&main_pass_cb_.Lights[0].Direction);
  XMVECTOR reflectedLightDir = XMVector3TransformNormal(lightDir, R);
	XMStoreFloat3(&mReflectedPassCB.Lights[0].Direction, reflectedLightDir);

	// Reflected pass stored in index 1
	auto currPassCB = current_frame_resource_->PassCB.get();
	currPassCB->CopyData(1, mReflectedPassCB);
}

void RenderSystem::UpdateMaterialBuffer(const GameTimer& gt) {
  auto material_cb = current_frame_resource_->MaterialBuffer.get();

  for (auto& pair : materials_) {
    auto material = pair.second.get();
    if (material->NumFrameDirty > 0) {

      XMMATRIX matTransform = XMLoadFloat4x4(&material->MatTransform);

      MaterialData mat_data;
      mat_data.Roughness = material->Roughness;
      mat_data.FresnelR0 = material->FresnelR0;
      mat_data.DiffuseAlbedo = material->DiffuseAlbedo;
      XMStoreFloat4x4(&mat_data.MatTransform, XMMatrixTranspose(matTransform));
      mat_data.DiffuseMapIndex = material->DiffuseSrvHeapIndex;

      material_cb->CopyData(material->MatCBIndex, mat_data);
      material->NumFrameDirty --;
    }
  }
}

void RenderSystem::AnimateMaterials(const GameTimer& gt)
{
	// Scroll the water material texture coordinates.
	auto waterMat = materials_["water"].get();

	float& tu = waterMat->MatTransform(3, 0);
	float& tv = waterMat->MatTransform(3, 1);

	tu += 0.1f * gt.DeltaTime();
	tv += 0.02f * gt.DeltaTime();

	if(tu >= 1.0f)
		tu -= 1.0f;

	if(tv >= 1.0f)
		tv -= 1.0f;
  //std::cout << "tu = " <<  tu << std::endl;
	waterMat->MatTransform(3, 0) = tu;
	waterMat->MatTransform(3, 1) = tv;

	// Material has changed, so need to update cbuffer.
	waterMat->NumFrameDirty = kNumFrameResources;
}

bool RenderSystem::Initialize() {
  if (!Application::Initialize())
    return false;

  ThrowIfFailed(d3d_command_list_->Reset(d3d_command_allocator_.Get(), nullptr));

  waves_ = std::make_unique<Waves>(128, 128, 1.0f, 0.03f, 4.0f, 0.2f);

  cbv_srv_uav_descriptor_size = d3d_device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
  blur_filter_ = std::make_unique<BlurFilter>(d3d_device_.Get(), client_width_, client_height_, DXGI_FORMAT_R8G8B8A8_UNORM);

  camera_.SetPosition(0.0f, 2.0f, -15.0f);

  BuildRootSignature();
  BuildPostProcessRootSignature();
  BuildShadersAndInputLayout();
  BuildShapeGeometry();
  //  BuildLandGeometry();
  BuildSkullGeometry();
  BuildRoomGeometry();
  //  BuildWavesGeometry();
  BuildMaterial();
  LoadTexture();
  BuildDescriptorHeaps();

  BuildRenderItems();
  BuildFrameResource();
  //  BuildDescriptorHeaps();
  //  BuildConstBufferView();
  BuildPipelineStateObjects();

  ThrowIfFailed(d3d_command_list_->Close());
	ID3D12CommandList* cmdsLists[] = { d3d_command_list_.Get() };
	d3d_command_queue_->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

  FlushCommandQueue();
  return true;
}

void RenderSystem::BuildFrameResource() {

  for (size_t i=0; i<kNumFrameResources; ++i) {
    frame_resources_.push_back(std::make_unique<FrameResource>(d3d_device_.Get(), 1, 
      (UINT)render_items_.size(), (UINT)materials_.size(), waves_->VertexCount()));
  }

}

void RenderSystem::BuildShapeGeometry() {
  GeometryGenerator geo_generator;
  auto mesh_box = geo_generator.CreateBox(1.0f, 1.0f, 1.0f, 0);

  vector<Vertex> vertices_geo(mesh_box.Vertices.size());
  vector<uint16_t> indices_geo = mesh_box.GetIndices16();
  for (int i=0; i<mesh_box.Vertices.size(); ++i) {
    vertices_geo[i].Pos = mesh_box.Vertices[i].Position;
    vertices_geo[i].Normal =  mesh_box.Vertices[i].Normal;
    vertices_geo[i].TexCoord =  mesh_box.Vertices[i].TexC;
  }

  //for (int i=0; i<mesh_sphere.Vertices.size(); ++i, ++k) {
  //  vertices_geo[k].Pos = mesh_sphere.Vertices[i].Position;
  //  vertices_geo[k].Color = XMFLOAT4(DirectX::Colors::Gold);
  //}

  //vector<std::uint16_t> indices_geo;
  //indices_geo.insert(indices_geo.end(), std::begin(mesh_box.GetIndices16()), std::end(mesh_box.GetIndices16()));
  //indices_geo.insert(indices_geo.end(), std::begin(mesh_sphere.GetIndices16()), std::end(mesh_sphere.GetIndices16()));

  UINT vertex_byte_size = (UINT)vertices_geo.size() * sizeof(Vertex);
  UINT index_byte_size = (UINT)indices_geo.size() * sizeof(std::uint16_t);


  auto geo = make_unique<MeshGeometry>();
  geo->Name = "ShapeGeo";

  ThrowIfFailed(D3DCreateBlob(vertex_byte_size, geo->VertexBufferCpu.GetAddressOf()));
  CopyMemory(geo->VertexBufferCpu->GetBufferPointer(), vertices_geo.data(), vertex_byte_size);
  
  geo->VertexBufferGpu = d3dUtil::CreateDefaultBuffer(d3d_device_.Get(), d3d_command_list_.Get(),
    vertices_geo.data(), vertex_byte_size, geo->VertexBufferUpload);

  ThrowIfFailed(D3DCreateBlob(index_byte_size, geo->IndexBufferCpu.GetAddressOf()));
  CopyMemory(geo->IndexBufferCpu->GetBufferPointer(), indices_geo.data(), index_byte_size);
  
  geo->IndexBufferGpu = d3dUtil::CreateDefaultBuffer(d3d_device_.Get(), d3d_command_list_.Get(),
    indices_geo.data(), index_byte_size, geo->IndexBufferUpload);

  geo->VertexBufferSize = vertex_byte_size;
  geo->VertexByteStride = (UINT)sizeof(Vertex);

  geo->IndexBufferSize = index_byte_size;

  SubmeshGeometry box_sub_mesh_geo;
  box_sub_mesh_geo.IndexCount = (UINT)indices_geo.size();
  box_sub_mesh_geo.BaseVertexLocation = 0;
  box_sub_mesh_geo.StartIndexLocation = 0;

  ////  geo->IndexCount = (UINT)indices_geo.size();

  geo->DrawArgs["Box"] = std::move(box_sub_mesh_geo);
  //geo->DrawArgs["Sphere"] = std::move(sphere_sub_mesh_geo);
  //
  mesh_geos_[geo->Name] = std::move(geo);
}


void RenderSystem::BuildLandGeometry() {
  GeometryGenerator geo_generator;
  auto mesh_grid = geo_generator.CreateGrid(160.0f, 160.0f, 50, 50);
  vector<Vertex> vertices_geo;
  vector<uint16_t> indices_geo = mesh_grid.GetIndices16();

  for (int i=0; i<mesh_grid.Vertices.size(); ++i) {
    auto& p = mesh_grid.Vertices[i].Position;
    Vertex v;
    v.Pos = p;
    v.Pos.y = GetHillsHeight(p.x, p.z);
    v.Normal = GetHillsNormal(p.x, p.z);
    v.TexCoord = mesh_grid.Vertices[i].TexC;
    //  v.Color = XMFLOAT4(DirectX::Colors::DarkCyan);

    // Color the vertex based on its height.
    //if(v.Pos.y < -10.0f)
    //{
    //    // Sandy beach color.
    //    v.Color = XMFLOAT4(1.0f, 0.96f, 0.62f, 1.0f);
    //}
    //else if(v.Pos.y < 5.0f)
    //{
    //    // Light yellow-green.
    //    v.Color = XMFLOAT4(0.48f, 0.77f, 0.46f, 1.0f);
    //}
    //else if(v.Pos.y < 12.0f)
    //{
    //    // Dark yellow-green.
    //    v.Color = XMFLOAT4(0.1f, 0.48f, 0.19f, 1.0f);
    //}
    //else if(v.Pos.y < 20.0f)
    //{
    //    // Dark brown.
    //    v.Color = XMFLOAT4(0.45f, 0.39f, 0.34f, 1.0f);
    //}
    //else
    //{
    //    // White snow.
    //    v.Color = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    //}

    vertices_geo.emplace_back(v);
  }

  auto geo = make_unique<MeshGeometry>();
  geo->Name = "Land";

  UINT vertex_byte_size = (UINT)vertices_geo.size() * sizeof(Vertex);

  ThrowIfFailed(D3DCreateBlob(vertex_byte_size, geo->VertexBufferCpu.GetAddressOf()));
  CopyMemory(geo->VertexBufferCpu->GetBufferPointer(), vertices_geo.data(), vertex_byte_size);

  geo->VertexBufferGpu = d3dUtil::CreateDefaultBuffer(d3d_device_.Get(), d3d_command_list_.Get(),
    vertices_geo.data(), vertex_byte_size, geo->VertexBufferUpload);

  UINT index_byte_size = (UINT)indices_geo.size() * sizeof(uint16_t);

  ThrowIfFailed(D3DCreateBlob(index_byte_size, geo->IndexBufferCpu.GetAddressOf()));
  CopyMemory(geo->IndexBufferCpu->GetBufferPointer(), indices_geo.data(), index_byte_size);
  
  geo->IndexBufferGpu = d3dUtil::CreateDefaultBuffer(d3d_device_.Get(), d3d_command_list_.Get(),
    indices_geo.data(), index_byte_size, geo->IndexBufferUpload);

  geo->VertexBufferSize = vertex_byte_size;
  geo->VertexByteStride = (UINT)sizeof(Vertex);

  geo->IndexBufferSize = index_byte_size;

  SubmeshGeometry grid_sub_mesh_geo;
  grid_sub_mesh_geo.IndexCount = (UINT)indices_geo.size();
  grid_sub_mesh_geo.BaseVertexLocation = 0;
  grid_sub_mesh_geo.StartIndexLocation = 0;

  geo->DrawArgs["Land"] = std::move(grid_sub_mesh_geo);
  
  mesh_geos_[geo->Name] = std::move(geo);

}

void RenderSystem::BuildWavesGeometry() {
  vector<uint16_t> indices_geo (waves_->TriangleCount() * 3);

  int m = waves_->RowCount();
  int n = waves_->ColumnCount();
  int k=0; 

  for (int i=0; i<m-1; ++i) {
    for (int j=0; j<n-1; ++j) {
      indices_geo[k] = i*n + j;
      indices_geo[k + 1] = i*n + j + 1;
      indices_geo[k + 2] = (i + 1)*n + j;

      indices_geo[k + 3] = (i + 1)*n + j;
      indices_geo[k + 4] = i*n + j + 1;
      indices_geo[k + 5] = (i + 1)*n + j + 1;

      k += 6;
    }
  }

  //  UINT vertex_byte_size = vertices_geo.size() * sizeof(Vertex);
  UINT vertex_byte_size = waves_->VertexCount() * sizeof(Vertex);
  UINT index_byte_size = (UINT)indices_geo.size() * sizeof(uint16_t);

  auto geo = make_unique<MeshGeometry>();
  geo->Name = "Waves";

  geo->VertexBufferCpu = nullptr;
  geo->VertexBufferGpu = nullptr;

  

  /*ThrowIfFailed(D3DCreateBlob(vertex_byte_size, geo->VertexBufferCpu.GetAddressOf()));
  CopyMemory(geo->VertexBufferCpu->GetBufferPointer(), vertices_geo.data(), vertex_byte_size);

  geo->VertexBufferGpu = d3dUtil::CreateDefaultBuffer(d3d_device_.Get(), d3d_command_list_.Get(),
    vertices_geo.data(), vertex_byte_size, geo->VertexBufferUpload);*/

  ThrowIfFailed(D3DCreateBlob(index_byte_size, geo->IndexBufferCpu.GetAddressOf()));
  CopyMemory(geo->IndexBufferCpu->GetBufferPointer(), indices_geo.data(), index_byte_size);

  geo->IndexBufferGpu = d3dUtil::CreateDefaultBuffer(d3d_device_.Get(), d3d_command_list_.Get(),
    indices_geo.data(), index_byte_size, geo->IndexBufferUpload);

  geo->VertexBufferSize = vertex_byte_size;
  geo->VertexByteStride = sizeof(Vertex);
  geo->IndexBufferSize = index_byte_size;
  geo->IndexFormat =  DXGI_FORMAT::DXGI_FORMAT_R16_UINT;

  SubmeshGeometry wave_sub_mesh_geo;
  wave_sub_mesh_geo.IndexCount = (UINT)indices_geo.size();
  wave_sub_mesh_geo.BaseVertexLocation = 0;
  wave_sub_mesh_geo.StartIndexLocation = 0;

  geo->DrawArgs["Waves"] = std::move(wave_sub_mesh_geo);

  mesh_geos_[geo->Name] = std::move(geo);
}

void RenderSystem::BuildSkullGeometry() {
  std::ifstream fin("Models/skull.txt");

  UINT vertex_count = 0;
  UINT index_count = 0;
  std::string ignore;

  if(!fin)
	{
		MessageBox(0, L"Models/skull.txt not found.", 0, 0);
		return;
	}

  fin >> ignore >> vertex_count;
  fin >> ignore >> index_count;
  fin >> ignore >> ignore >> ignore >> ignore;

  vector<Vertex> vertices_geo(vertex_count);
  vector<uint16_t> indices_geo (index_count * 3);

  for (UINT i=0; i < vertex_count; ++i) {
    fin >> vertices_geo[i].Pos.x >> vertices_geo[i].Pos.y >> vertices_geo[i].Pos.z;
    fin >> vertices_geo[i].Normal.x >> vertices_geo[i].Normal.y >> vertices_geo[i].Normal.z;
    vertices_geo[i].TexCoord = {0.0f, 0.0f};
  }

  fin >> ignore >> ignore >> ignore;

  for (UINT i=0; i < index_count; ++i) {
    fin >> indices_geo[i*3+0] >> indices_geo[i*3+1] >> indices_geo[i*3+2];
  }

  fin.close();

  auto geo = make_unique<MeshGeometry>();
  geo->Name = "skullGeo";

  UINT vertex_byte_size = (UINT)vertices_geo.size() * sizeof(Vertex);

  ThrowIfFailed(D3DCreateBlob(vertex_byte_size, geo->VertexBufferCpu.GetAddressOf()));
  CopyMemory(geo->VertexBufferCpu->GetBufferPointer(), vertices_geo.data(), vertex_byte_size);

  geo->VertexBufferGpu = d3dUtil::CreateDefaultBuffer(d3d_device_.Get(), d3d_command_list_.Get(),
    vertices_geo.data(), vertex_byte_size, geo->VertexBufferUpload);

  UINT index_byte_size = (UINT)indices_geo.size() * sizeof(uint16_t);

  ThrowIfFailed(D3DCreateBlob(index_byte_size, geo->IndexBufferCpu.GetAddressOf()));
  CopyMemory(geo->IndexBufferCpu->GetBufferPointer(), indices_geo.data(), index_byte_size);
  
  geo->IndexBufferGpu = d3dUtil::CreateDefaultBuffer(d3d_device_.Get(), d3d_command_list_.Get(),
    indices_geo.data(), index_byte_size, geo->IndexBufferUpload);

  geo->VertexBufferSize = vertex_byte_size;
  geo->VertexByteStride = (UINT)sizeof(Vertex);

  geo->IndexBufferSize = index_byte_size;

  SubmeshGeometry skull_sub_mesh_geo;
  skull_sub_mesh_geo.IndexCount = (UINT)indices_geo.size();
  skull_sub_mesh_geo.BaseVertexLocation = 0;
  skull_sub_mesh_geo.StartIndexLocation = 0;

  geo->DrawArgs["skull"] = std::move(skull_sub_mesh_geo);
  
  mesh_geos_[geo->Name] = std::move(geo);

  //  ---------------

//  UINT vcount = 0;
//	UINT tcount = 0;
////	std::string ignore;
//
//	fin >> ignore >> vcount;
//	fin >> ignore >> tcount;
//	//  fin >> ignore >> ignore >> ignore >> ignore;
//  fin >> ignore;
//  fin >> ignore;
//  fin >> ignore;
//  fin >> ignore;
//	
//	std::vector<Vertex> vertices(vcount);
//	for(UINT i = 0; i < vcount; ++i)
//	{
//		fin >> vertices[i].Pos.x >> vertices[i].Pos.y >> vertices[i].Pos.z;
//		fin >> vertices[i].Normal.x >> vertices[i].Normal.y >> vertices[i].Normal.z;
//
//		// Model does not have texture coordinates, so just zero them out.
//		vertices[i].TexCoord = { 0.0f, 0.0f };
//	}
//
//	fin >> ignore;
//	fin >> ignore;
//	fin >> ignore;
//
//	std::vector<std::int32_t> indices(3 * tcount);
//	for(UINT i = 0; i < tcount; ++i)
//	{
//		fin >> indices[i*3+0] >> indices[i*3+1] >> indices[i*3+2];
//	}
//
//	fin.close();
// 
//	//
//	// Pack the indices of all the meshes into one index buffer.
//	//
// 
//	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
//
//	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::int32_t);
//
//	auto geo = std::make_unique<MeshGeometry>();
//	geo->Name = "skullGeo";
//
//	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCpu));
//	CopyMemory(geo->VertexBufferCpu->GetBufferPointer(), vertices.data(), vbByteSize);
//
//	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCpu));
//	CopyMemory(geo->IndexBufferCpu->GetBufferPointer(), indices.data(), ibByteSize);
//
//	geo->VertexBufferGpu = d3dUtil::CreateDefaultBuffer(d3d_device_.Get(),
//		d3d_command_list_.Get(), vertices.data(), vbByteSize, geo->VertexBufferUpload);
//
//	geo->IndexBufferGpu = d3dUtil::CreateDefaultBuffer(d3d_device_.Get(),
//		d3d_command_list_.Get(), indices.data(), ibByteSize, geo->IndexBufferUpload);
//
//	geo->VertexByteStride = sizeof(Vertex);
//	geo->VertexBufferSize = vbByteSize;
//	geo->IndexFormat = DXGI_FORMAT_R32_UINT;
//	geo->IndexBufferSize = ibByteSize;
//
//	SubmeshGeometry submesh;
//	submesh.IndexCount = (UINT)indices.size();
//	submesh.StartIndexLocation = 0;
//	submesh.BaseVertexLocation = 0;
//
//	geo->DrawArgs["skull"] = submesh;
//
//	mesh_geos_[geo->Name] = std::move(geo);

}

void RenderSystem::BuildRoomGeometry()
{
    	// Create and specify geometry.  For this sample we draw a floor
	// and a wall with a mirror on it.  We put the floor, wall, and
	// mirror geometry in one vertex buffer.
	//
	//   |--------------|
	//   |              |
    //   |----|----|----|
    //   |Wall|Mirr|Wall|
	//   |    | or |    |
    //   /--------------/
    //  /   Floor      /
	// /--------------/

	std::array<Vertex, 20> vertices = 
	{
		// Floor: Observe we tile texture coordinates.
		Vertex(-3.5f, 0.0f, -10.0f, 0.0f, 1.0f, 0.0f, 0.0f, 4.0f), // 0 
		Vertex(-3.5f, 0.0f,   0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f),
		Vertex(7.5f, 0.0f,   0.0f, 0.0f, 1.0f, 0.0f, 4.0f, 0.0f),
		Vertex(7.5f, 0.0f, -10.0f, 0.0f, 1.0f, 0.0f, 4.0f, 4.0f),

		// Wall: Observe we tile texture coordinates, and that we
		// leave a gap in the middle for the mirror.
		Vertex(-3.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 2.0f), // 4
		Vertex(-3.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f),
		Vertex(-2.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.5f, 0.0f),
		Vertex(-2.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.5f, 2.0f),

		Vertex(2.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 2.0f), // 8 
		Vertex(2.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f),
		Vertex(7.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 2.0f, 0.0f),
		Vertex(7.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 2.0f, 2.0f),

		Vertex(-3.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f), // 12
		Vertex(-3.5f, 6.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f),
		Vertex(7.5f, 6.0f, 0.0f, 0.0f, 0.0f, -1.0f, 6.0f, 0.0f),
		Vertex(7.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 6.0f, 1.0f),

		// Mirror
		Vertex(-2.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f), // 16
		Vertex(-2.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f),
		Vertex(2.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f),
		Vertex(2.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f)
	};

	std::array<std::int16_t, 30> indices = 
	{
		// Floor
		0, 1, 2,	
		0, 2, 3,

		// Walls
		4, 5, 6,
		4, 6, 7,

		8, 9, 10,
		8, 10, 11,

		12, 13, 14,
		12, 14, 15,

		// Mirror
		16, 17, 18,
		16, 18, 19
	};

	SubmeshGeometry floorSubmesh;
	floorSubmesh.IndexCount = 6;
	floorSubmesh.StartIndexLocation = 0;
	floorSubmesh.BaseVertexLocation = 0;

	SubmeshGeometry wallSubmesh;
	wallSubmesh.IndexCount = 18;
	wallSubmesh.StartIndexLocation = 6;
	wallSubmesh.BaseVertexLocation = 0;

	SubmeshGeometry mirrorSubmesh;
	mirrorSubmesh.IndexCount = 6;
	mirrorSubmesh.StartIndexLocation = 24;
	mirrorSubmesh.BaseVertexLocation = 0;

  const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
  const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "roomGeo";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCpu));
	CopyMemory(geo->VertexBufferCpu->GetBufferPointer(), vertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCpu));
	CopyMemory(geo->IndexBufferCpu->GetBufferPointer(), indices.data(), ibByteSize);

	geo->VertexBufferGpu = d3dUtil::CreateDefaultBuffer(d3d_device_.Get(),
		d3d_command_list_.Get(), vertices.data(), vbByteSize, geo->VertexBufferUpload);

	geo->IndexBufferGpu = d3dUtil::CreateDefaultBuffer(d3d_device_.Get(),
		d3d_command_list_.Get(), indices.data(), ibByteSize, geo->IndexBufferUpload);

	geo->VertexByteStride = sizeof(Vertex);
	geo->VertexBufferSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferSize = ibByteSize;

	geo->DrawArgs["floor"] = floorSubmesh;
	geo->DrawArgs["wall"] = wallSubmesh;
	geo->DrawArgs["mirror"] = mirrorSubmesh;

	mesh_geos_[geo->Name] = std::move(geo);
}

void RenderSystem::BuildRenderItems() {
  //auto box_render_item = std::make_unique<RenderItem>();
  //box_render_item->Geo = mesh_geos_["ShapeGeo"].get();
  //box_render_item->Mat = materials_["woodCrate"].get();
  //box_render_item->ObjectCBIndex = 0;
  //
  //XMStoreFloat4x4(&box_render_item->World, XMMatrixScaling(1.0f, 1.0f, 1.0f) * XMMatrixTranslation(0.0f, 0.0f, 0.0f));
  //box_render_item->IndexCount = box_render_item->Geo->DrawArgs["Box"].IndexCount;
  //box_render_item->StartIndexLocation = box_render_item->Geo->DrawArgs["Box"].StartIndexLocation;
  //box_render_item->BaseVertexLocation = box_render_item->Geo->DrawArgs["Box"].BaseVertexLocation;
  //box_render_item->PrimitiveType = D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
  ////  
  //

  //auto grid_render_item = std::make_unique<RenderItem>();
  //grid_render_item->Geo = mesh_geos_["Land"].get();
  //grid_render_item->Mat = materials_["woodCrate"].get();
  //grid_render_item->ObjectCBIndex = 1;
  //XMStoreFloat4x4(&grid_render_item->World, XMMatrixScaling(1.0f, 1.0f, 1.0f) * XMMatrixTranslation(0.0f, 0.0f, 0.0f));
  //grid_render_item->IndexCount = grid_render_item->Geo->DrawArgs["Land"].IndexCount;
  //grid_render_item->StartIndexLocation = grid_render_item->Geo->DrawArgs["Land"].StartIndexLocation;
  //grid_render_item->BaseVertexLocation = grid_render_item->Geo->DrawArgs["Land"].BaseVertexLocation;
  //grid_render_item->PrimitiveType = D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
  //
  //////  
  //auto wave_render_item = std::make_unique<RenderItem>();
  //wave_render_item->Geo = mesh_geos_["Waves"].get();
  //wave_render_item->Mat = materials_["water"].get();
  //wave_render_item->ObjectCBIndex = 1;
  //wave_render_item->World = MathHelper::Identity4x4();
  //XMStoreFloat4x4(&wave_render_item->TexTransform, XMMatrixScaling(5.0f, 5.0f, 1.0f));
  ////  XMStoreFloat4x4(&wave_render_item->World, XMMatrixScaling(1.0f, 1.0f, 1.0f) * XMMatrixTranslation(0.0f, 0.0f, 0.0f));
  ////  XMStoreFloat4x4(&wave_render_item->World, XMMatrixScaling(1.0f, 1.0f, 1.0f) * XMMatrixTranslation(0.0f, 0.0f, 0.0f));
  //wave_render_item->IndexCount = wave_render_item->Geo->DrawArgs["Waves"].IndexCount;
  //wave_render_item->StartIndexLocation = wave_render_item->Geo->DrawArgs["Waves"].StartIndexLocation;
  //wave_render_item->BaseVertexLocation = wave_render_item->Geo->DrawArgs["Waves"].BaseVertexLocation;
  //wave_render_item->PrimitiveType = D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

  //waves_render_item_ = wave_render_item.get();

  //auto skull_render_item = std::make_unique<RenderItem>();
  //skull_render_item->Geo = mesh_geos_["Skull"].get();
  //skull_render_item->Mat = materials_["woodCrate"].get();
  //skull_render_item->ObjectCBIndex = 0;
  //XMStoreFloat4x4(&skull_render_item->World, XMMatrixScaling(1.0f, 1.0f, 1.0f) * XMMatrixTranslation(0.0f, 0.0f, 0.0f));
  //skull_render_item->IndexCount = skull_render_item->Geo->DrawArgs["Skull"].IndexCount;
  //skull_render_item->StartIndexLocation = skull_render_item->Geo->DrawArgs["Skull"].StartIndexLocation;
  //skull_render_item->BaseVertexLocation = skull_render_item->Geo->DrawArgs["Skull"].BaseVertexLocation;
  //skull_render_item->PrimitiveType = D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
  //items_layers_[(int)RenderLayer::kOpaque].push_back(skull_render_item.get());

  auto floorRitem = std::make_unique<RenderItem>();
	floorRitem->World = MathHelper::Identity4x4();
	floorRitem->TexTransform = MathHelper::Identity4x4();
	floorRitem->ObjectCBIndex = 0;
	floorRitem->Mat = materials_["checkertile"].get();
	floorRitem->Geo = mesh_geos_["roomGeo"].get();
	floorRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	floorRitem->IndexCount = floorRitem->Geo->DrawArgs["floor"].IndexCount;
	floorRitem->StartIndexLocation = floorRitem->Geo->DrawArgs["floor"].StartIndexLocation;
	floorRitem->BaseVertexLocation = floorRitem->Geo->DrawArgs["floor"].BaseVertexLocation;
	items_layers_[(int)RenderLayer::kOpaque].push_back(floorRitem.get());

    auto wallsRitem = std::make_unique<RenderItem>();
	wallsRitem->World = MathHelper::Identity4x4();
	wallsRitem->TexTransform = MathHelper::Identity4x4();
	wallsRitem->ObjectCBIndex = 1;
	wallsRitem->Mat = materials_["bricks"].get();
	wallsRitem->Geo = mesh_geos_["roomGeo"].get();
	wallsRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	wallsRitem->IndexCount = wallsRitem->Geo->DrawArgs["wall"].IndexCount;
	wallsRitem->StartIndexLocation = wallsRitem->Geo->DrawArgs["wall"].StartIndexLocation;
	wallsRitem->BaseVertexLocation = wallsRitem->Geo->DrawArgs["wall"].BaseVertexLocation;
	items_layers_[(int)RenderLayer::kOpaque].push_back(wallsRitem.get());


  XMMATRIX skullRotate = XMMatrixRotationY(0.5f*MathHelper::Pi);
	XMMATRIX skullScale = XMMatrixScaling(0.45f, 0.45f, 0.45f);
	XMMATRIX skullOffset = XMMatrixTranslation(-1.0f, 0.0f, -4.0f);
  XMMATRIX skull_world = skullRotate * skullScale * skullOffset;

	auto skullRitem = std::make_unique<RenderItem>();
	//  skullRitem->World = MathHelper::Identity4x4();

  XMStoreFloat4x4(&skullRitem->World, skull_world);
	skullRitem->TexTransform = MathHelper::Identity4x4();
	skullRitem->ObjectCBIndex = 2;
	skullRitem->Mat = materials_["skullMat"].get();
	skullRitem->Geo = mesh_geos_["skullGeo"].get();
	skullRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	skullRitem->IndexCount = skullRitem->Geo->DrawArgs["skull"].IndexCount;
	skullRitem->StartIndexLocation = skullRitem->Geo->DrawArgs["skull"].StartIndexLocation;
	skullRitem->BaseVertexLocation = skullRitem->Geo->DrawArgs["skull"].BaseVertexLocation;
	mSkullRitem = skullRitem.get();
	items_layers_[(int)RenderLayer::kOpaque].push_back(skullRitem.get());

	// Reflected skull will have different world matrix, so it needs to be its own render item.
  // Update reflection world matrix.
	XMVECTOR mirrorPlane = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f); // xy plane
	XMMATRIX R = XMMatrixReflect(mirrorPlane);
	
	auto reflectedSkullRitem = std::make_unique<RenderItem>();
	*reflectedSkullRitem = *skullRitem;
  XMStoreFloat4x4(&reflectedSkullRitem->World, skull_world * R);
  //  XMStoreFloat4x4(&skullRitem->World, XMMatrixScaling(0.5f,0.5f,0.5f) * XMMatrixRotationAxis({0.0f, 1.0f, 0.0f}, 90.0f) * XMMatrixTranslation(-1.0f, 0.0f, 4.0f));
	reflectedSkullRitem->ObjectCBIndex = 3;
	mReflectedSkullRitem = reflectedSkullRitem.get();
	items_layers_[(int)RenderLayer::kReflected].push_back(reflectedSkullRitem.get());

	// Shadowed skull will have different world matrix, so it needs to be its own render item.
	auto shadowedSkullRitem = std::make_unique<RenderItem>();
	*shadowedSkullRitem = *skullRitem;
	shadowedSkullRitem->ObjectCBIndex = 4;
	shadowedSkullRitem->Mat = materials_["shadowMat"].get();
	mShadowedSkullRitem = shadowedSkullRitem.get();
	items_layers_[(int)RenderLayer::kOpaque].push_back(shadowedSkullRitem.get());

	auto mirrorRitem = std::make_unique<RenderItem>();
	mirrorRitem->World = MathHelper::Identity4x4();
	mirrorRitem->TexTransform = MathHelper::Identity4x4();
	mirrorRitem->ObjectCBIndex = 5;
	mirrorRitem->Mat = materials_["icemirror"].get();
	mirrorRitem->Geo = mesh_geos_["roomGeo"].get();
	mirrorRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	mirrorRitem->IndexCount = mirrorRitem->Geo->DrawArgs["mirror"].IndexCount;
	mirrorRitem->StartIndexLocation = mirrorRitem->Geo->DrawArgs["mirror"].StartIndexLocation;
	mirrorRitem->BaseVertexLocation = mirrorRitem->Geo->DrawArgs["mirror"].BaseVertexLocation;
	items_layers_[(int)RenderLayer::kMirrors].push_back(mirrorRitem.get());
	items_layers_[(int)RenderLayer::kTransparent].push_back(mirrorRitem.get());

	render_items_.push_back(std::move(floorRitem));
	render_items_.push_back(std::move(wallsRitem));
	render_items_.push_back(std::move(skullRitem));
	render_items_.push_back(std::move(reflectedSkullRitem));
	render_items_.push_back(std::move(shadowedSkullRitem));
	render_items_.push_back(std::move(mirrorRitem));

  //  --------------

  

  //render_items_.push_back(std::move(skull_render_item));
}

void RenderSystem::DrawRenderItems() {
  

  //for (auto item : opaque_items_) {
  //  d3d_command_list_->IASetVertexBuffers(0, 1, &item->Geo->VertexBufferView());
  //  d3d_command_list_->IASetIndexBuffer(&item->Geo->IndexBufferView());
  //  d3d_command_list_->IASetPrimitiveTopology(item->PrimitiveType);

  //  int object_cbv_index = frame_resource_index_ * (UINT)opaque_items_.size() + item->ObjectCBIndex;
  //  auto handle = CD3DX12_GPU_DESCRIPTOR_HANDLE(cbv_heap_->GetGPUDescriptorHandleForHeapStart());
  //  handle.Offset(object_cbv_index, cbv_srv_uav_descriptor_size);


  //  d3d_command_list_->SetGraphicsRootDescriptorTable(0, cbv_heap_->GetGPUDescriptorHandleForHeapStart());

  //  d3d_command_list_->DrawIndexedInstanced(item->IndexCount, 1, item->StartIndexLocation, item->BaseVertexLocation, 0);
  //}
}

void RenderSystem::BuildShadersAndInputLayout() {
  shaders_["standardVS"] = d3dUtil::CompileShader(L"Shaders\\default.hlsl", nullptr, "VS", "vs_5_1");
	shaders_["opaquePS"] = d3dUtil::CompileShader(L"Shaders\\default.hlsl", nullptr, "PS", "ps_5_1");
  shaders_["horzBlurCS"] = d3dUtil::CompileShader(L"Shaders\\Blur.hlsl", nullptr, "HorzBlurCS", "cs_5_0");
	shaders_["vertBlurCS"] = d3dUtil::CompileShader(L"Shaders\\Blur.hlsl", nullptr, "VertBlurCS", "cs_5_0");
	
  input_layout_ =
  {
      { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
      { "NORMAL", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
      { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
  };
}

void RenderSystem::BuildRootSignature() {
  CD3DX12_DESCRIPTOR_RANGE tex_table;
	tex_table.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4, 0, 0);

  CD3DX12_ROOT_PARAMETER slotRootParameter[4];
  
  slotRootParameter[0].InitAsConstantBufferView(0);
  slotRootParameter[1].InitAsConstantBufferView(1);
    slotRootParameter[2].InitAsShaderResourceView(0, 1);
  //slotRootParameter[2].InitAsConstantBufferView(2);
  slotRootParameter[3].InitAsDescriptorTable(1, &tex_table, D3D12_SHADER_VISIBILITY_PIXEL);

  auto static_samplers = GetStaticSamplers();

  CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(4, slotRootParameter, 
    (UINT)static_samplers.size(), static_samplers.data(), 
      D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);


	// create a root signature with a single slot which points to a descriptor range consisting of a single constant buffer
	ComPtr<ID3DBlob> serializedRootSig = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
		serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

	if(errorBlob != nullptr)
	{
		::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}
	ThrowIfFailed(hr);

	ThrowIfFailed(d3d_device_->CreateRootSignature(
		0,
		serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(),
		IID_PPV_ARGS(root_signature_.GetAddressOf())));
}

void RenderSystem::BuildPostProcessRootSignature() {
  CD3DX12_DESCRIPTOR_RANGE srvTable;
	srvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

	CD3DX12_DESCRIPTOR_RANGE uavTable;
	uavTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);

	// Root parameter can be a table, root descriptor or root constants.
	CD3DX12_ROOT_PARAMETER slotRootParameter[3];

	// Perfomance TIP: Order from most frequent to least frequent.
	slotRootParameter[0].InitAsConstants(12, 0);
	slotRootParameter[1].InitAsDescriptorTable(1, &srvTable);
	slotRootParameter[2].InitAsDescriptorTable(1, &uavTable);

	// A root signature is an array of root parameters.
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(3, slotRootParameter,
		0, nullptr,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	// create a root signature with a single slot which points to a descriptor range consisting of a single constant buffer
	ComPtr<ID3DBlob> serializedRootSig = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
		serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

	if(errorBlob != nullptr)
	{
		::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}
	ThrowIfFailed(hr);

	ThrowIfFailed(d3d_device_->CreateRootSignature(
		0,
		serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(),
		IID_PPV_ARGS(post_process_root_signature_.GetAddressOf())));
}

void RenderSystem::BuildDescriptorHeaps() {
  const int textureDescriptorCount = 4;
	const int blurDescriptorCount = 4;

  D3D12_DESCRIPTOR_HEAP_DESC src_heap_desc = {};
  src_heap_desc.NumDescriptors = textureDescriptorCount + blurDescriptorCount;
  src_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
  
  src_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAGS::D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

  ThrowIfFailed(d3d_device_->CreateDescriptorHeap(&src_heap_desc, IID_PPV_ARGS(srv_descriptor_heap_.GetAddressOf()) ));

  //////-----------------------------

  

	//
	// Create the SRV heap.
	//
	//D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	//srvHeapDesc.NumDescriptors = textureDescriptorCount + blurDescriptorCount;
 // //  srvHeapDesc.NumDescriptors = textureDescriptorCount;
	//srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	//srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	//ThrowIfFailed(d3d_device_->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&srv_descriptor_heap_)));

  CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(srv_descriptor_heap_->GetCPUDescriptorHandleForHeapStart());

  //auto wood_crate_tex = textures_["Wood"]->Resource;
  //auto grass_tex = textures_["Grass"]->Resource;
  //auto water1_tex = textures_["Water"]->Resource;

  auto bricksTex = textures_["bricksTex"]->Resource;
	auto checkboardTex = textures_["checkboardTex"]->Resource;
	auto iceTex = textures_["iceTex"]->Resource;
	auto white1x1Tex = textures_["white1x1Tex"]->Resource;

  int offsetCount = 0;
  D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
	srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srv_desc.Format = bricksTex->GetDesc().Format;
	srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srv_desc.Texture2D.MostDetailedMip = 0;
	srv_desc.Texture2D.MipLevels = -1;  //  wood_crate_tex->GetDesc().MipLevels;
	srv_desc.Texture2D.ResourceMinLODClamp = 0.0f;

  d3d_device_->CreateShaderResourceView(bricksTex.Get(), &srv_desc, hDescriptor);
  ++offsetCount;

  hDescriptor.Offset(1, cbv_srv_uav_descriptor_size);
  srv_desc.Format = checkboardTex->GetDesc().Format;
  d3d_device_->CreateShaderResourceView(checkboardTex.Get(), &srv_desc, hDescriptor);
  ++offsetCount;

  hDescriptor.Offset(1, cbv_srv_uav_descriptor_size);
  srv_desc.Format = iceTex->GetDesc().Format;
  d3d_device_->CreateShaderResourceView(iceTex.Get(), &srv_desc, hDescriptor);
  ++offsetCount;

  hDescriptor.Offset(1, cbv_srv_uav_descriptor_size);
  srv_desc.Format = white1x1Tex->GetDesc().Format;
  d3d_device_->CreateShaderResourceView(white1x1Tex.Get(), &srv_desc, hDescriptor);
  ++offsetCount;

  blur_filter_->BuildDescriptors(
		CD3DX12_CPU_DESCRIPTOR_HANDLE(srv_descriptor_heap_->GetCPUDescriptorHandleForHeapStart(), 4, cbv_srv_uav_descriptor_size),
		CD3DX12_GPU_DESCRIPTOR_HANDLE(srv_descriptor_heap_->GetGPUDescriptorHandleForHeapStart(), 4, cbv_srv_uav_descriptor_size),
		cbv_srv_uav_descriptor_size);
}

void RenderSystem::BuildPipelineStateObjects() {
  D3D12_GRAPHICS_PIPELINE_STATE_DESC pso_desc;

  ZeroMemory(&pso_desc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
  pso_desc.InputLayout = { input_layout_.data(), (UINT)input_layout_.size() };
  pso_desc.VS = 
	{ 
		/*reinterpret_cast<BYTE*>(vertex_shader_code_->GetBufferPointer()), 
		vertex_shader_code_->GetBufferSize() */
    reinterpret_cast<BYTE*>(shaders_["standardVS"]->GetBufferPointer()), 
		shaders_["standardVS"]->GetBufferSize() 
	};
  pso_desc.PS = 
	{ 
		/*reinterpret_cast<BYTE*>(pixel_shader_code_->GetBufferPointer()), 
		pixel_shader_code_->GetBufferSize() */
    reinterpret_cast<BYTE*>(shaders_["opaquePS"]->GetBufferPointer()), 
		shaders_["opaquePS"]->GetBufferSize() 

	};
  pso_desc.pRootSignature = root_signature_.Get();
  pso_desc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
  pso_desc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
  pso_desc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
  pso_desc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
  pso_desc.SampleMask = UINT_MAX;
  pso_desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
  pso_desc.NumRenderTargets = 1;
  pso_desc.RTVFormats[0] = back_buffer_format_;
  pso_desc.DSVFormat = depth_stencil_format_;
  pso_desc.SampleDesc.Quality = use_4x_msaa_state_ ? (use_4x_msaa_quanlity_-1) : 0;
  pso_desc.SampleDesc.Count = use_4x_msaa_state_ ? 4 : 1;

  //  ThrowIfFailed(d3d_device_->CreateGraphicsPipelineState(&pso_desc, IID_PPV_ARGS(&pipeline_state_object_)));
  ThrowIfFailed(d3d_device_->CreateGraphicsPipelineState(&pso_desc, IID_PPV_ARGS(&pipeline_state_objects_["Opaque"])));

  D3D12_GRAPHICS_PIPELINE_STATE_DESC transparentPsoDesc = pso_desc;
  D3D12_RENDER_TARGET_BLEND_DESC transparencyBlendDesc;
  transparencyBlendDesc.BlendEnable = true;
  transparencyBlendDesc.LogicOpEnable = false;
  transparencyBlendDesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
  transparencyBlendDesc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
  transparencyBlendDesc.BlendOp = D3D12_BLEND_OP_ADD;
  transparencyBlendDesc.SrcBlendAlpha = D3D12_BLEND_ONE;
  transparencyBlendDesc.DestBlendAlpha = D3D12_BLEND_ZERO;
  transparencyBlendDesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
  transparencyBlendDesc.LogicOp = D3D12_LOGIC_OP_NOOP;
  transparencyBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
  transparentPsoDesc.BlendState.RenderTarget[0] = transparencyBlendDesc;
  ThrowIfFailed(d3d_device_->CreateGraphicsPipelineState(&transparentPsoDesc, IID_PPV_ARGS(&pipeline_state_objects_[pso_name_transparent])));

  D3D12_DEPTH_STENCIL_DESC mirrorDss;
  mirrorDss.DepthEnable = true;
  mirrorDss.DepthWriteMask = D3D12_DEPTH_WRITE_MASK::D3D12_DEPTH_WRITE_MASK_ZERO;
  mirrorDss.DepthFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_LESS;
  mirrorDss.StencilEnable = true;
  mirrorDss.StencilReadMask = 0xff;
  mirrorDss.StencilWriteMask = 0xff;

  mirrorDss.FrontFace.StencilFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
  mirrorDss.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
  mirrorDss.FrontFace.StencilPassOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_REPLACE;
  mirrorDss.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_ALWAYS;

  mirrorDss.BackFace.StencilFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
  mirrorDss.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
  mirrorDss.BackFace.StencilPassOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_REPLACE;
  mirrorDss.BackFace.StencilFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_ALWAYS;

  D3D12_GRAPHICS_PIPELINE_STATE_DESC mirrorPsoDesc = pso_desc;
  mirrorPsoDesc.DepthStencilState = mirrorDss;
  //mirrorPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
  //mirrorPsoDesc.RasterizerState.FrontCounterClockwise = true;
  ThrowIfFailed(d3d_device_->CreateGraphicsPipelineState(&mirrorPsoDesc, IID_PPV_ARGS(&pipeline_state_objects_[pso_name_mirror])));


  D3D12_DEPTH_STENCIL_DESC reflectionDss;
  reflectionDss.DepthEnable = true;
  reflectionDss.DepthWriteMask = D3D12_DEPTH_WRITE_MASK::D3D12_DEPTH_WRITE_MASK_ALL;
  reflectionDss.DepthFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_LESS;
  reflectionDss.StencilEnable = true;
  reflectionDss.StencilReadMask = 0xff;
  reflectionDss.StencilWriteMask = 0xff;

  reflectionDss.FrontFace.StencilFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
  reflectionDss.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
  reflectionDss.FrontFace.StencilPassOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
  reflectionDss.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_EQUAL;

  reflectionDss.BackFace.StencilFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
  reflectionDss.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
  reflectionDss.BackFace.StencilPassOp = D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
  reflectionDss.BackFace.StencilFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_EQUAL;

  D3D12_GRAPHICS_PIPELINE_STATE_DESC reflectionPsoDesc = pso_desc;
  reflectionPsoDesc.DepthStencilState = reflectionDss;
  reflectionPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
  reflectionPsoDesc.RasterizerState.FrontCounterClockwise = true;
  ThrowIfFailed(d3d_device_->CreateGraphicsPipelineState(&reflectionPsoDesc, IID_PPV_ARGS(&pipeline_state_objects_[pso_name_reflection])));


  D3D12_COMPUTE_PIPELINE_STATE_DESC horzBlurPSO = {};
	horzBlurPSO.pRootSignature = post_process_root_signature_.Get();
	horzBlurPSO.CS =
	{
		reinterpret_cast<BYTE*>(shaders_["horzBlurCS"]->GetBufferPointer()),
		shaders_["horzBlurCS"]->GetBufferSize()
	};
	horzBlurPSO.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	ThrowIfFailed(d3d_device_->CreateComputePipelineState(&horzBlurPSO, IID_PPV_ARGS(&pipeline_state_objects_["horzBlur"])));


  D3D12_COMPUTE_PIPELINE_STATE_DESC vertBlurPSO = {};
  vertBlurPSO.pRootSignature = post_process_root_signature_.Get();
  vertBlurPSO.CS = 
	{ 
    reinterpret_cast<BYTE*>(shaders_["vertBlurCS"]->GetBufferPointer()), 
		shaders_["vertBlurCS"]->GetBufferSize() 
	};
  vertBlurPSO.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
  ThrowIfFailed(d3d_device_->CreateComputePipelineState(&vertBlurPSO, IID_PPV_ARGS(&pipeline_state_objects_["vertBlur"])));
}

void RenderSystem::OnResize() {
  Application::OnResize();

  XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f*MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
    XMStoreFloat4x4(&mProj, P);

  if (blur_filter_ != nullptr) {
    blur_filter_->OnResize(client_width_, client_height_);
  }

  camera_.SetLens(0.25f*MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
}

void RenderSystem::OnKeyboardInput(const GameTimer& gt) {
  const float dt = gt.DeltaTime();

	if(GetAsyncKeyState('W') & 0x8000)
		camera_.Walk(10.0f*dt);

	if(GetAsyncKeyState('S') & 0x8000)
		camera_.Walk(-10.0f*dt);

	if(GetAsyncKeyState('A') & 0x8000)
		camera_.Strafe(-10.0f*dt);

	if(GetAsyncKeyState('D') & 0x8000)
		camera_.Strafe(10.0f*dt);

	camera_.UpdateViewMatrix();
}

void RenderSystem::OnMouseDown(WPARAM btnState, int x, int y) {
  last_mouse_pos.x = x;
  last_mouse_pos.y = y;

  SetCapture(h_window_);
}

void RenderSystem::OnMouseUp(WPARAM btnState, int x, int y) {
  ReleaseCapture();
}

void RenderSystem::OnMouseMove(WPARAM btnState, int x, int y) {
  if((btnState & MK_LBUTTON) != 0) {
    float speed = 0.25f;
    auto dx = XMConvertToRadians(speed * static_cast<float>( x - last_mouse_pos.x));
    auto dy = XMConvertToRadians(speed * static_cast<float>(y - last_mouse_pos.y));

    //  mTheta += dx;
    //  mPhi += dy;

    camera_.Pitch(dy);
    camera_.RotateY(dx);

    //mTheta = MathHelper::Clamp(mTheta, 0.1f, MathHelper::Pi - 0.1f);
    //  mPhi = MathHelper::Clamp(mPhi, 0.1f, MathHelper::Pi - 0.1f);
  }
  //else if((btnState & MK_RBUTTON) != 0)
  //{
  //    // Make each pixel correspond to 0.2 unit in the scene.
  //    float dx = 0.2f*static_cast<float>(x - last_mouse_pos.x);
  //    float dy = 0.2f*static_cast<float>(y - last_mouse_pos.y);

  //    // Update the camera radius based on input.
  //    mRadius += dx - dy;

  //    // Restrict the radius.
  //    mRadius = MathHelper::Clamp(mRadius, 5.0f, 150.0f);
  //}

  last_mouse_pos.x = x;
  last_mouse_pos.y = y;
}


void RenderSystem::BuildMaterial() {
 // auto grass = std::make_unique<Material>();
	//grass->Name = "grass";
	//grass->MatCBIndex = 0;
 // grass->DiffuseSrvHeapIndex = 1;
 // //  grass->DiffuseAlbedo = XMFLOAT4(0.2f, 0.6f, 0.2f, 1.0f);
 // grass->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
 // 
 // grass->FresnelR0 = XMFLOAT3(0.01f, 0.01f, 0.01f);
 // grass->Roughness = 0.125f;

 //   // This is not a good water material definition, but we do not have all the rendering
 //   // tools we need (transparency, environment reflection), so we fake it for now.
	//auto water = std::make_unique<Material>();
	//water->Name = "water";
	//water->MatCBIndex = 1;
 // water->DiffuseSrvHeapIndex = 2;
 // water->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 0.5f);
 // water->FresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
 // water->Roughness = 0.0f;

 // auto wood_crate = std::make_unique<Material>();
	//wood_crate->Name = "woodCrate";
	//wood_crate->MatCBIndex = 2;
	//wood_crate->DiffuseSrvHeapIndex = 0;
	//wood_crate->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	//wood_crate->FresnelR0 = XMFLOAT3(0.05f, 0.05f, 0.05f);
	//wood_crate->Roughness = 0.2f;

	//materials_["woodCrate"] = std::move(wood_crate);

	//materials_["grass"] = std::move(grass);
	//materials_["water"] = std::move(water);

  auto bricks = std::make_unique<Material>();
	bricks->Name = "bricks";
	bricks->MatCBIndex = 0;
	bricks->DiffuseSrvHeapIndex = 0;
	bricks->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	bricks->FresnelR0 = XMFLOAT3(0.05f, 0.05f, 0.05f);
	bricks->Roughness = 0.25f;

	auto checkertile = std::make_unique<Material>();
	checkertile->Name = "checkertile";
	checkertile->MatCBIndex = 1;
	checkertile->DiffuseSrvHeapIndex = 1;
	checkertile->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	checkertile->FresnelR0 = XMFLOAT3(0.07f, 0.07f, 0.07f);
	checkertile->Roughness = 0.3f;

	auto icemirror = std::make_unique<Material>();
	icemirror->Name = "icemirror";
	icemirror->MatCBIndex = 2;
	icemirror->DiffuseSrvHeapIndex = 2;
	icemirror->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 0.3f);
	icemirror->FresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
	icemirror->Roughness = 0.5f;

	auto skullMat = std::make_unique<Material>();
	skullMat->Name = "skullMat";
	skullMat->MatCBIndex = 3;
	skullMat->DiffuseSrvHeapIndex = 3;
	skullMat->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	skullMat->FresnelR0 = XMFLOAT3(0.05f, 0.05f, 0.05f);
	skullMat->Roughness = 0.3f;

	auto shadowMat = std::make_unique<Material>();
	shadowMat->Name = "shadowMat";
	shadowMat->MatCBIndex = 4;
	shadowMat->DiffuseSrvHeapIndex = 3;
	shadowMat->DiffuseAlbedo = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.5f);
	shadowMat->FresnelR0 = XMFLOAT3(0.001f, 0.001f, 0.001f);
	shadowMat->Roughness = 0.0f;

	materials_["bricks"] = std::move(bricks);
	materials_["checkertile"] = std::move(checkertile);
	materials_["icemirror"] = std::move(icemirror);
	materials_["skullMat"] = std::move(skullMat);
	materials_["shadowMat"] = std::move(shadowMat);
  
}

void RenderSystem::LoadTexture() {
  /*auto woodCrateTex = make_unique<Texture>();
  woodCrateTex->Name = "wood";
  woodCrateTex->Filename = L"Textures\\WoodCrate01.dds";

  ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(d3d_device_.Get(),
		d3d_command_list_.Get(), woodCrateTex->Filename.c_str(),
		woodCrateTex->Resource, woodCrateTex->UploadHeap));

  textures_["Wood"] = move(woodCrateTex);

  auto grass_tex = make_unique<Texture>();
  grass_tex->Name = "grass";
  grass_tex->Filename = L"Textures\\grass.dds";

  ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(d3d_device_.Get(),
		d3d_command_list_.Get(), grass_tex->Filename.c_str(),
		grass_tex->Resource, grass_tex->UploadHeap));

  textures_["Grass"] = move(grass_tex);

  auto water1_tex = make_unique<Texture>();
  water1_tex->Name = "water1";
  water1_tex->Filename = L"Textures\\water1.dds";

  ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(d3d_device_.Get(),
		d3d_command_list_.Get(), water1_tex->Filename.c_str(),
		water1_tex->Resource, water1_tex->UploadHeap));

  textures_["Water"] = move(water1_tex);*/

  //  ----------------

  auto bricksTex = std::make_unique<Texture>();
	bricksTex->Name = "bricksTex";
	bricksTex->Filename = L"Textures/bricks3.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(d3d_device_.Get(),
		d3d_command_list_.Get(), bricksTex->Filename.c_str(),
		bricksTex->Resource, bricksTex->UploadHeap));

	auto checkboardTex = std::make_unique<Texture>();
	checkboardTex->Name = "checkboardTex";
	checkboardTex->Filename = L"Textures/checkboard.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(d3d_device_.Get(),
		d3d_command_list_.Get(), checkboardTex->Filename.c_str(),
		checkboardTex->Resource, checkboardTex->UploadHeap));

	auto iceTex = std::make_unique<Texture>();
	iceTex->Name = "iceTex";
	iceTex->Filename = L"Textures/ice.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(d3d_device_.Get(),
		d3d_command_list_.Get(), iceTex->Filename.c_str(),
		iceTex->Resource, iceTex->UploadHeap));

	auto white1x1Tex = std::make_unique<Texture>();
	white1x1Tex->Name = "white1x1Tex";
	white1x1Tex->Filename = L"Textures/white1x1.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(d3d_device_.Get(),
		d3d_command_list_.Get(), white1x1Tex->Filename.c_str(),
		white1x1Tex->Resource, white1x1Tex->UploadHeap));

	textures_[bricksTex->Name] = std::move(bricksTex);
	textures_[checkboardTex->Name] = std::move(checkboardTex);
	textures_[iceTex->Name] = std::move(iceTex);
	textures_[white1x1Tex->Name] = std::move(white1x1Tex);
}

float RenderSystem::GetHillsHeight(float x, float z)const
{
    return 0.3f*(z*sinf(0.1f*x) + x*cosf(0.1f*z));
}

XMFLOAT3 RenderSystem::GetHillsNormal(float x, float z)const
{
    // n = (-df/dx, 1, -df/dz)
    XMFLOAT3 n(
        -0.03f*z*cosf(0.1f*x) - 0.3f*cosf(0.1f*z),
        1.0f,
        -0.3f*sinf(0.1f*x) + 0.03f*x*sinf(0.1f*z));

    XMVECTOR unitNormal = XMVector3Normalize(XMLoadFloat3(&n));
    XMStoreFloat3(&n, unitNormal);

    return n;
}

std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> RenderSystem::GetStaticSamplers()
{
	// Applications usually only need a handful of samplers.  So just define them all up front
	// and keep them available as part of the root signature.  

	const CD3DX12_STATIC_SAMPLER_DESC pointWrap(
		0, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC pointClamp(
		1, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC linearWrap(
		2, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC linearClamp(
		3, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC anisotropicWrap(
		4, // shaderRegister
		D3D12_FILTER_ANISOTROPIC, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressW
		0.0f,                             // mipLODBias
		8);                               // maxAnisotropy

	const CD3DX12_STATIC_SAMPLER_DESC anisotropicClamp(
		5, // shaderRegister
		D3D12_FILTER_ANISOTROPIC, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressW
		0.0f,                              // mipLODBias
		8);                                // maxAnisotropy

	return { 
		pointWrap, pointClamp,
		linearWrap, linearClamp, 
		anisotropicWrap, anisotropicClamp };
}


}


