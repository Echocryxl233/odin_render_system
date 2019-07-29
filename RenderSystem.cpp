#include "RenderSystem.h"
#include "GeometryGenerator.h"
#include <iostream> 

using namespace std;

namespace OdinRenderSystem {

RenderSystem::RenderSystem(HINSTANCE h_instance)
  :Application(h_instance)
{
  // Estimate the scene bounding sphere manually since we know how the scene was constructed.
// The grid is the "widest object" with a width of 20 and depth of 30.0f, and centered at
// the world space origin.  In general, you need to loop over every world space vertex
// position and compute the bounding sphere.
  mSceneBounds.Center = XMFLOAT3(0.0f, 0.0f, 0.0f);
  mSceneBounds.Radius = sqrtf(10.0f * 10.0f + 15.0f * 15.0f);
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
  ThrowIfFailed(d3d_command_list_->Reset(cmdListAlloc.Get(), psos_["opaque"].Get()));

  ID3D12DescriptorHeap* descriptorHeaps[] = { srv_descriptor_heap_.Get() };
  d3d_command_list_->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

  d3d_command_list_->SetGraphicsRootSignature(root_signature_.Get());

  auto material_buffer = current_frame_resource_->MaterialBuffer->Resource();
  d3d_command_list_->SetGraphicsRootShaderResourceView(0, material_buffer->GetGPUVirtualAddress());

  CD3DX12_GPU_DESCRIPTOR_HANDLE sky_tex_descriptor(srv_descriptor_heap_->GetGPUDescriptorHandleForHeapStart());
  sky_tex_descriptor.Offset(sky_heap_index_, cbv_srv_uav_descriptor_size);
  d3d_command_list_->SetGraphicsRootDescriptorTable(3, sky_tex_descriptor);

  ////  CD3DX12_GPU_DESCRIPTOR_HANDLE
  //CD3DX12_GPU_DESCRIPTOR_HANDLE debug_descriptor(srv_descriptor_heap_->GetGPUDescriptorHandleForHeapStart());
  //debug_descriptor.Offset(4, cbv_srv_uav_descriptor_size);
  //d3d_command_list_->SetGraphicsRootDescriptorTable(4, debug_descriptor);
  ////  d3d_command_list_->SetGraphicsRootDescriptorTable(4, shadow_map_->Dsv());
  ////  d3d_command_list_->SetGraphicsRootDescriptorTable(4, srv_descriptor_heap_->GetGPUDescriptorHandleForHeapStart());
  ////  d3d_command_list_->SetGraphicsRootDescriptorTable(4, shadow_map_->Srv());

  d3d_command_list_->SetGraphicsRootDescriptorTable(3, null_cube_srv_handle_gpu);

  d3d_command_list_->SetGraphicsRootDescriptorTable(4, srv_descriptor_heap_->GetGPUDescriptorHandleForHeapStart());

  //  DrawSceneToCubeMap();

  DrawSceneToShadowMap();

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

  UINT passCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(PassConstants));
  auto passCB = current_frame_resource_->PassCB->Resource();
  d3d_command_list_->SetGraphicsRootConstantBufferView(2, passCB->GetGPUVirtualAddress());

  //CD3DX12_GPU_DESCRIPTOR_HANDLE dynamic_tex_descriptor(srv_descriptor_heap_->GetGPUDescriptorHandleForHeapStart());
  //dynamic_tex_descriptor.Offset(dynamic_heap_index_, cbv_srv_uav_descriptor_size);
  //d3d_command_list_->SetGraphicsRootDescriptorTable(3, dynamic_tex_descriptor);

  d3d_command_list_->SetGraphicsRootDescriptorTable(3, sky_tex_descriptor);

  d3d_command_list_->SetPipelineState(psos_["opaque"].Get());
  DrawRenderItems(d3d_command_list_.Get(), (int)RenderLayer::kReflected);

  //  d3d_command_list_->SetGraphicsRootDescriptorTable(3, sky_tex_descriptor);
  DrawRenderItems(d3d_command_list_.Get(), (int)RenderLayer::kOpaque);

  DrawRenderItems(d3d_command_list_.Get(), (int)RenderLayer::kShadow);

  d3d_command_list_->SetPipelineState(psos_["debug"].Get());
  DrawRenderItems(d3d_command_list_.Get(), (int)RenderLayer::kDebug);

  d3d_command_list_->SetPipelineState(psos_["opaqueNormal"].Get());
  DrawRenderItems(d3d_command_list_.Get(), (int)RenderLayer::kMirrors);

  d3d_command_list_->SetPipelineState(psos_["sky"].Get());
  DrawRenderItems(d3d_command_list_.Get(), (int)RenderLayer::kSky);

  //d3d_command_list_->OMSetStencilRef(1);
  //d3d_command_list_->SetPipelineState(pipeline_state_objects_[pso_name_mirror].Get());
  //DrawRenderItems(d3d_command_list_.Get(), items_layers_[(int)RenderLayer::kMirrors]);

  ////  d3d_command_list_->SetGraphicsRootConstantBufferView(3, passCB->GetGPUVirtualAddress() + 1*passCBByteSize);
  //d3d_command_list_->SetPipelineState(pipeline_state_objects_[pso_name_reflection].Get());
  //DrawRenderItems(d3d_command_list_.Get(), items_layers_[(int)RenderLayer::kReflected]);
  //

  //  d3d_command_list_->SetGraphicsRootConstantBufferView(3, passCB->GetGPUVirtualAddress());
  //  d3d_command_list_->OMSetStencilRef(0);

  d3d_command_list_->SetPipelineState(psos_["Transparent"].Get());
  DrawRenderItems(d3d_command_list_.Get(), (int)RenderLayer::kTransparent);
  //  DrawRenderItems(d3d_command_list_.Get(), items_layers_[(int)RenderLayer::kTransparent]);

  //  blur_filter_->Execute(d3d_command_list_.Get(), post_process_root_signature_.Get(), 
  //    pipeline_state_objects_["horzBlur"].Get(), pipeline_state_objects_["vertBlur"].Get(), CurrentBackBuffer(), 4);

  //  d3d_command_list_->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
  //    D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_COPY_DEST));

  //  d3d_command_list_->CopyResource(CurrentBackBuffer(), blur_filter_->Output());

  // Transition to PRESENT state.
  //  d3d_command_list_->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
  //    D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PRESENT));

  // Indicate a state transition on the resource usage.
  d3d_command_list_->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
    D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

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
  auto instanceBuffer = current_frame_resource_->InstanceBufferGroups[(int)RenderLayer::kOpaque]->Resource();

  // For each render item...
  for(size_t i = 0; i < ritems.size(); ++i)
  {
    auto ri = ritems[i];

    cmdList->IASetVertexBuffers(0, 1, &ri->Geo->VertexBufferView());
    cmdList->IASetIndexBuffer(&ri->Geo->IndexBufferView());
    cmdList->IASetPrimitiveTopology(ri->PrimitiveType);

    d3d_command_list_->SetGraphicsRootShaderResourceView(1, instanceBuffer->GetGPUVirtualAddress());
    cmdList->DrawIndexedInstanced(ri->IndexCount, ri->InstanceCount, ri->StartIndexLocation, ri->BaseVertexLocation, 0);
  }
}

void RenderSystem::DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const int render_layer)
{
  UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));
  UINT matCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(MaterialConstants));

  auto objectCB = current_frame_resource_->ObjectCB->Resource();
  auto materialCB = current_frame_resource_->MaterialBuffer->Resource();
  auto instanceBuffer = current_frame_resource_->InstanceBufferGroups[render_layer]->Resource();

  // For each render item...
  auto layer_render_items = items_layers_[render_layer];
  for (size_t i = 0; i < layer_render_items.size(); ++i)
  {
    auto ri = layer_render_items[i];

    cmdList->IASetVertexBuffers(0, 1, &ri->Geo->VertexBufferView());
    cmdList->IASetIndexBuffer(&ri->Geo->IndexBufferView());
    cmdList->IASetPrimitiveTopology(ri->PrimitiveType);

    d3d_command_list_->SetGraphicsRootShaderResourceView(1, instanceBuffer->GetGPUVirtualAddress());
    cmdList->DrawIndexedInstanced(ri->IndexCount, ri->InstanceCount, ri->StartIndexLocation, ri->BaseVertexLocation, 0);
  }
}


void RenderSystem::DrawSceneToCubeMap() {
  d3d_command_list_->RSSetViewports(1, &dynamic_cube_map_->Viewport());
  d3d_command_list_->RSSetScissorRects(1, &dynamic_cube_map_->ScissorRect());

  d3d_command_list_->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(dynamic_cube_map_->Resource(),
    D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET));

  auto current_pass_cb = current_frame_resource_->PassCB->Resource();
  int pass_cb_byte_size = d3dUtil::CalcConstantBufferByteSize(sizeof(PassConstants));

  for (int i = 0; i < 6; ++i) {
    d3d_command_list_->ClearRenderTargetView(dynamic_cube_map_->Rtv(i), Colors::LightSteelBlue, 0, nullptr);
    d3d_command_list_->ClearDepthStencilView(cube_dsv_, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

    d3d_command_list_->OMSetRenderTargets(1, &dynamic_cube_map_->Rtv(i), true, &cube_dsv_);

    D3D12_GPU_VIRTUAL_ADDRESS passCBAddress = current_pass_cb->GetGPUVirtualAddress() + (1 + i) * pass_cb_byte_size;
    d3d_command_list_->SetGraphicsRootConstantBufferView(2, passCBAddress);

    DrawRenderItems(d3d_command_list_.Get(), (int)RenderLayer::kOpaque);

    DrawRenderItems(d3d_command_list_.Get(), (int)RenderLayer::kMirrors);

    d3d_command_list_->SetPipelineState(psos_["sky"].Get());
    DrawRenderItems(d3d_command_list_.Get(), (int)RenderLayer::kSky);

    d3d_command_list_->SetPipelineState(psos_["Transparent"].Get());
    DrawRenderItems(d3d_command_list_.Get(), (int)RenderLayer::kTransparent);

    d3d_command_list_->SetPipelineState(psos_["opaque"].Get());
  }

  d3d_command_list_->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(dynamic_cube_map_->Resource(),
    D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ));
}

void RenderSystem::DrawSceneToShadowMap() {
  d3d_command_list_->RSSetViewports(1, &shadow_map_->Viewport());
  d3d_command_list_->RSSetScissorRects(1, &shadow_map_->ScissorRect());

  d3d_command_list_->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(shadow_map_->Resource(),
    D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_DEPTH_WRITE));

  d3d_command_list_->ClearDepthStencilView(shadow_map_->Dsv(),
    D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

  d3d_command_list_->OMSetRenderTargets(0, nullptr, true, &shadow_map_->Dsv());

  auto current_pass_cb = current_frame_resource_->PassCB->Resource();
  int pass_cb_byte_size = d3dUtil::CalcConstantBufferByteSize(sizeof(PassConstants));
  D3D12_GPU_VIRTUAL_ADDRESS passCBAddress = current_pass_cb->GetGPUVirtualAddress() + 0 * pass_cb_byte_size;  
  
  d3d_command_list_->SetGraphicsRootConstantBufferView(2, passCBAddress);

  d3d_command_list_->SetPipelineState(psos_["shadow_opaque"].Get());

  //  DrawRenderItems(d3d_command_list_.Get(), mRitemLayer[(int)RenderLayer::Opaque]);

  DrawRenderItems(d3d_command_list_.Get(), (int)RenderLayer::kOpaque);

  d3d_command_list_->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(shadow_map_->Resource(),
    D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_GENERIC_READ));
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
  //  UpdateSkullAnimate(gt);
  UpdateObjectCBs(gt);
  UpdateInstanceData(gt);
  UpdateMainPassCB(gt);
  //  UpdateCubeMapFacePassCBs(gt);
  UpdateMaterialBuffer(gt);
  //  UpdateReflectedPassCB(gt);
  //  UpdateWave(gt);
  
}

void RenderSystem::UpdateSkullAnimate(const GameTimer& gt) {

  int index = 0;
  for (auto& instance : skull_render_item_->Instances) {
    //  XMStoreFloat4x4(&instance.World, 
    XMMATRIX self_transform = XMLoadFloat4x4(&instance.World);
   
    XMMATRIX rotate_self = XMMatrixRotationY(2.0f*gt.DeltaTime());
    XMMATRIX scale = XMMatrixScaling(1.0f, 1.0f, 1.0f);
    XMMATRIX offset = XMMatrixTranslation(10.0f, 0.0f, -0.0f);
    
    XMMATRIX world = rotate_self * self_transform * rotate_self;
    //  XMMATRIX world = self_transform * scale * rotate_self * offset;

    XMStoreFloat4x4(&instance.World, world);

    skull_render_item_->NumFrameDirty = kNumFrameResources;

    ++index;
  }
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

  XMMATRIX shadowTransform = XMLoadFloat4x4(&mShadowTransform);

	XMStoreFloat4x4(&main_pass_cb_.View, XMMatrixTranspose(view));
	XMStoreFloat4x4(&main_pass_cb_.InvView, XMMatrixTranspose(invView));
	XMStoreFloat4x4(&main_pass_cb_.Proj, XMMatrixTranspose(proj));
	XMStoreFloat4x4(&main_pass_cb_.InvProj, XMMatrixTranspose(invProj));
	XMStoreFloat4x4(&main_pass_cb_.ViewProj, XMMatrixTranspose(viewProj));
	XMStoreFloat4x4(&main_pass_cb_.InvViewProj, XMMatrixTranspose(invViewProj));
  XMStoreFloat4x4(&main_pass_cb_.ShadowTransform, XMMatrixTranspose(shadowTransform));
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
  //  UpdateCubeMapFacePassCBs(gt);
  //  UpdateCubeMapFacePassCBs();
}

void RenderSystem::UpdateShadowPassCB(const GameTimer& gt)
{
  XMMATRIX view = XMLoadFloat4x4(&mLightView);
  XMMATRIX proj = XMLoadFloat4x4(&mLightProj);

  XMMATRIX viewProj = XMMatrixMultiply(view, proj);
  XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
  XMMATRIX invProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);
  XMMATRIX invViewProj = XMMatrixInverse(&XMMatrixDeterminant(viewProj), viewProj);

  UINT w = shadow_map_->Width();
  UINT h = shadow_map_->Height();

  XMStoreFloat4x4(&mShadowPassCB.View, XMMatrixTranspose(view));
  XMStoreFloat4x4(&mShadowPassCB.InvView, XMMatrixTranspose(invView));
  XMStoreFloat4x4(&mShadowPassCB.Proj, XMMatrixTranspose(proj));
  XMStoreFloat4x4(&mShadowPassCB.InvProj, XMMatrixTranspose(invProj));
  XMStoreFloat4x4(&mShadowPassCB.ViewProj, XMMatrixTranspose(viewProj));
  XMStoreFloat4x4(&mShadowPassCB.InvViewProj, XMMatrixTranspose(invViewProj));
  mShadowPassCB.EyePosW = mLightPosW;
  mShadowPassCB.RenderTargetSize = XMFLOAT2((float)w, (float)h);
  mShadowPassCB.InvRenderTargetSize = XMFLOAT2(1.0f / w, 1.0f / h);
  mShadowPassCB.NearZ = mLightNearZ;
  mShadowPassCB.FarZ = mLightFarZ;

  auto currPassCB = current_frame_resource_->PassCB.get();
  currPassCB->CopyData(1, mShadowPassCB);
}

void RenderSystem::UpdateCubeMapFacePassCBs(const GameTimer& gt) {
  const int pass_cb_index_offset = 1; //  the main pass cb is 0, so cube map pass cb start from 1
  auto current_pass_cb = current_frame_resource_->PassCB.get();

  for (int i = 0; i < 6; ++i) {
    auto cube_face_pass_cb = main_pass_cb_;
    Camera current_camera = cube_map_cameras_[i];

    XMMATRIX view = current_camera.GetView();
    XMMATRIX proj = current_camera.GetProj();

    XMMATRIX viewProj = XMMatrixMultiply(view, proj);
    XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
    XMMATRIX invProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);
    XMMATRIX invViewProj = XMMatrixInverse(&XMMatrixDeterminant(viewProj), viewProj);

    XMStoreFloat4x4(&cube_face_pass_cb.View, XMMatrixTranspose(view));
    XMStoreFloat4x4(&cube_face_pass_cb.InvView, XMMatrixTranspose(invView));
    XMStoreFloat4x4(&cube_face_pass_cb.Proj, XMMatrixTranspose(proj));
    XMStoreFloat4x4(&cube_face_pass_cb.InvProj, XMMatrixTranspose(invProj));
    XMStoreFloat4x4(&cube_face_pass_cb.ViewProj, XMMatrixTranspose(viewProj));
    XMStoreFloat4x4(&cube_face_pass_cb.InvViewProj, XMMatrixTranspose(invViewProj));

    cube_face_pass_cb.EyePosW = current_camera.GetPosition3f();
    cube_face_pass_cb.RenderTargetSize = XMFLOAT2((float)kCubeMapSize, (float)kCubeMapSize);
    cube_face_pass_cb.InvRenderTargetSize = XMFLOAT2(1.0f / kCubeMapSize, 1.0f / kCubeMapSize);

    // Cube map pass cbuffers are stored in elements 1-6.
    current_pass_cb->CopyData(i + pass_cb_index_offset, cube_face_pass_cb);
  }
}


void RenderSystem::UpdateCubeMapFacePassCBs()
{
  PassConstants cubeFacePassCB = main_pass_cb_;
  for (int i = 0; i < 6; ++i)
  {
    Camera current_camera = cube_map_cameras_[i];

    XMMATRIX view = cube_map_cameras_[i].GetView();
    XMMATRIX proj = cube_map_cameras_[i].GetProj();

    XMMATRIX viewProj = XMMatrixMultiply(view, proj);
    XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
    XMMATRIX invProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);
    XMMATRIX invViewProj = XMMatrixInverse(&XMMatrixDeterminant(viewProj), viewProj);

    XMStoreFloat4x4(&cubeFacePassCB.View, XMMatrixTranspose(view));
    XMStoreFloat4x4(&cubeFacePassCB.InvView, XMMatrixTranspose(invView));
    XMStoreFloat4x4(&cubeFacePassCB.Proj, XMMatrixTranspose(proj));
    XMStoreFloat4x4(&cubeFacePassCB.InvProj, XMMatrixTranspose(invProj));
    XMStoreFloat4x4(&cubeFacePassCB.ViewProj, XMMatrixTranspose(viewProj));
    XMStoreFloat4x4(&cubeFacePassCB.InvViewProj, XMMatrixTranspose(invViewProj));
    cubeFacePassCB.EyePosW = current_camera.GetPosition3f();
    cubeFacePassCB.RenderTargetSize = XMFLOAT2((float)kCubeMapSize, (float)kCubeMapSize);
    cubeFacePassCB.InvRenderTargetSize = XMFLOAT2(1.0f / kCubeMapSize, 1.0f / kCubeMapSize);

    auto currPassCB = current_frame_resource_->PassCB.get();

    // Cube map pass cbuffers are stored in elements 1-6.
    currPassCB->CopyData(1 + i, cubeFacePassCB);
  }
}

void RenderSystem::UpdateShadowPassCB()
{
  PassConstants cubeFacePassCB = main_pass_cb_;

    //Camera current_camera = cube_map_cameras_[i];

    //XMMATRIX view = cube_map_cameras_[i].GetView();
    //XMMATRIX proj = cube_map_cameras_[i].GetProj();

    //XMMATRIX viewProj = XMMatrixMultiply(view, proj);
    //XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
    //XMMATRIX invProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);
    //XMMATRIX invViewProj = XMMatrixInverse(&XMMatrixDeterminant(viewProj), viewProj);

    //XMStoreFloat4x4(&cubeFacePassCB.View, XMMatrixTranspose(view));
    //XMStoreFloat4x4(&cubeFacePassCB.InvView, XMMatrixTranspose(invView));
    //XMStoreFloat4x4(&cubeFacePassCB.Proj, XMMatrixTranspose(proj));
    //XMStoreFloat4x4(&cubeFacePassCB.InvProj, XMMatrixTranspose(invProj));
    //XMStoreFloat4x4(&cubeFacePassCB.ViewProj, XMMatrixTranspose(viewProj));
    //XMStoreFloat4x4(&cubeFacePassCB.InvViewProj, XMMatrixTranspose(invViewProj));
    //cubeFacePassCB.EyePosW = current_camera.GetPosition3f();
    //cubeFacePassCB.RenderTargetSize = XMFLOAT2((float)kCubeMapSize, (float)kCubeMapSize);
    //cubeFacePassCB.InvRenderTargetSize = XMFLOAT2(1.0f / kCubeMapSize, 1.0f / kCubeMapSize);

    auto currPassCB = current_frame_resource_->PassCB.get();

    // Cube map pass cbuffers are stored in elements 1-6.
    currPassCB->CopyData(1, cubeFacePassCB);
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

void RenderSystem::UpdateInstanceData(const GameTimer& gt) {
  auto view = camera_.GetView();
  auto inv_view = XMMatrixInverse(&XMMatrixDeterminant(view), view);

  int visible_instance_count = 0;
  int total_visible_count = 0;
  size_t total_instance_count = 0;
 
  for (int i = (int)RenderLayer::kOpaque; i < (int)RenderLayer::kMaxCount; ++i) {

    auto layer_render_items = items_layers_[i];

    for (auto& item : layer_render_items) {
      auto current_instance_buffer = current_frame_resource_->InstanceBufferGroups[i].get();
      visible_instance_count = 0;

      const auto& instances = item->Instances;
      total_instance_count += instances.size();

      for (auto& instance_data : instances) {

        BoundingFrustum local_frustum;
        XMMATRIX world = XMLoadFloat4x4(&instance_data.World);
        XMMATRIX tex_transform = XMLoadFloat4x4(&instance_data.TexTransform);
        XMMATRIX inv_world = XMMatrixInverse(&XMMatrixDeterminant(world), world);
        XMMATRIX local_to_view = XMMatrixMultiply(inv_view, inv_world);

        camera_frustum_.Transform(local_frustum, local_to_view);

        if (local_frustum.Contains(item->Bounds) != ContainmentType::DISJOINT || (!enable_frustum_culling_) || item->Visible) {
          InstanceData data;
          XMStoreFloat4x4(&data.World, XMMatrixTranspose(world));  //  why is the transpose matrix of world
          XMStoreFloat4x4(&data.TexTransform, XMMatrixTranspose(tex_transform));  
          //  data.MaterialIndex = item->Mat->MatCBIndex; 
          data.MaterialIndex = instance_data.MaterialIndex;

          current_instance_buffer->CopyData(visible_instance_count, data);
          visible_instance_count++;
        }
      }

      item->InstanceCount = visible_instance_count;
      total_visible_count += visible_instance_count;
    }
  }
  

  std::wostringstream outs;
  outs.precision(0);
  outs << L"Odin Instancing" <<
    L"    " << total_visible_count <<
    L" objects visible out of " << total_instance_count;

  main_wnd_caption_ = outs.str();

  if (!use_mouse_)
    SetWindowText(h_window_, main_wnd_caption_.c_str());
}

void RenderSystem::UpdateMaterialBuffer(const GameTimer& gt) {
  auto material_cb = current_frame_resource_->MaterialBuffer.get();

  auto instance = MaterialManager::GetInstance();

  auto it = instance->Begin();

  //  for (auto& pair : materials_) {
  for (; it != instance->End(); ++it) {
    auto material = it->second.get();
    if (material->NumFrameDirty > 0) {
      XMMATRIX matTransform = XMLoadFloat4x4(&material->MatTransform);

      MaterialData mat_data;
      mat_data.Roughness = material->Roughness;
      mat_data.FresnelR0 = material->FresnelR0;
      mat_data.DiffuseAlbedo = material->DiffuseAlbedo;
      XMStoreFloat4x4(&mat_data.MatTransform, XMMatrixTranspose(matTransform));
      mat_data.DiffuseMapIndex = material->DiffuseSrvHeapIndex;
      mat_data.NormalMapIndex = material->NormalSrvHeapIndex;
      //  mat_data.DiffuseMapIndex = material->MatCBIndex;
      

      material_cb->CopyData(material->MatCBIndex, mat_data);
      material->NumFrameDirty --;
    }
  }
}



void RenderSystem::AnimateMaterials(const GameTimer& gt)
{
	// Scroll the water material texture coordinates.

	//  auto waterMat = materials_["water"].get();
  auto waterMat = MaterialManager::GetInstance()->GetMaterial("water");

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

  BuildCubeFaceCamera(0.0f, 2.0f, 0.0f);

  dynamic_cube_map_ = make_unique<CubeRenderTarget>(d3d_device_.Get(), kCubeMapSize,
    kCubeMapSize, DXGI_FORMAT_R8G8B8A8_UNORM);

  shadow_map_ = make_unique<ShadowMap>(d3d_device_.Get(), kCubeMapSize, kCubeMapSize);

  BuildRootSignature();
  BuildPostProcessRootSignature();
  BuildShadersAndInputLayout();
  BuildShapeGeometry();
  //  BuildLandGeometry();
  BuildSkullGeometry();
  BuildRoomGeometry();
  //  BuildWavesGeometry();

  LoadTexture();
  BuildDescriptorHeaps();
  BuildMaterial();
  //  BuildCubeDepthStencil();
  BuildRenderItems();
  BuildFrameResource();
  //  BuildDescriptorHeaps();
  //  BuildConstBufferView();
  BuildPSOs();

  ThrowIfFailed(d3d_command_list_->Close());
	ID3D12CommandList* cmdsLists[] = { d3d_command_list_.Get() };
	d3d_command_queue_->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

  FlushCommandQueue();
  return true;
}

void RenderSystem::CreateRtvAndDsvDescriptorHeaps() {
  D3D12_DESCRIPTOR_HEAP_DESC rtv_heap_desc;
  rtv_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
  rtv_heap_desc.NumDescriptors = kSwapBufferCount + dynamic_cube_map_->RTVCount();
  rtv_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAGS::D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
  rtv_heap_desc.NodeMask = 0;

  ThrowIfFailed(d3d_device_->CreateDescriptorHeap(&rtv_heap_desc,
    IID_PPV_ARGS(rtv_heap_.GetAddressOf())));

  D3D12_DESCRIPTOR_HEAP_DESC dsv_heap_desc;
  dsv_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
  dsv_heap_desc.NumDescriptors = 2;
  dsv_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAGS::D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
  dsv_heap_desc.NodeMask = 0;

  ThrowIfFailed(d3d_device_->CreateDescriptorHeap(&dsv_heap_desc,
    IID_PPV_ARGS(dsv_heap_.GetAddressOf())));

  cube_dsv_ = CD3DX12_CPU_DESCRIPTOR_HANDLE(
    dsv_heap_->GetCPUDescriptorHandleForHeapStart(),
    1,
    dsv_descriptor_size);
}

void RenderSystem::BuildCubeDepthStencil()
{
  // Create the depth/stencil buffer and view.
  D3D12_RESOURCE_DESC depthStencilDesc;
  depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
  depthStencilDesc.Alignment = 0;
  depthStencilDesc.Width = kCubeMapSize;
  depthStencilDesc.Height = kCubeMapSize;
  depthStencilDesc.DepthOrArraySize = 1;
  depthStencilDesc.MipLevels = 1;
  depthStencilDesc.Format = depth_stencil_format_;  // mDepthStencilFormat;
  depthStencilDesc.SampleDesc.Count = 1;
  depthStencilDesc.SampleDesc.Quality = 0;
  depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
  depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

  D3D12_CLEAR_VALUE optClear;
  optClear.Format = depth_stencil_format_;  // mDepthStencilFormat;
  optClear.DepthStencil.Depth = 1.0f;
  optClear.DepthStencil.Stencil = 0;
  ThrowIfFailed(d3d_device_->CreateCommittedResource(
    &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
    D3D12_HEAP_FLAG_NONE,
    &depthStencilDesc,
    D3D12_RESOURCE_STATE_COMMON, 
    &optClear,
    IID_PPV_ARGS(cube_depth_stencil_buffer_.GetAddressOf())));

  // Create descriptor to mip level 0 of entire resource using the format of the resource.
  d3d_device_->CreateDepthStencilView(cube_depth_stencil_buffer_.Get(), nullptr, cube_dsv_);

  // Transition the resource from its initial state to be used as a depth buffer.
   d3d_command_list_->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(cube_depth_stencil_buffer_.Get(),
    D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE));
}

void RenderSystem::BuildFrameResource() {

  for (size_t i=0; i<kNumFrameResources; ++i) {
    frame_resources_.push_back(std::make_unique<FrameResource>(d3d_device_.Get(), 1 + dynamic_cube_map_->RTVCount(),
      (UINT)render_items_.size(), instance_count_, 
      //  (UINT)materials_.size(), 
      (UINT)MaterialManager::GetInstance()->MaterialCount(),
      waves_->VertexCount()));
  }

}

void RenderSystem::BuildShapeGeometry() {
  GeometryGenerator geo_generator;
  auto mesh_shpere = geo_generator.CreateSphere(0.5f, 20, 20);

  vector<Vertex> vertices_geo(mesh_shpere.Vertices.size());
  vector<uint16_t> indices_geo = mesh_shpere.GetIndices16();
  for (int i=0; i<mesh_shpere.Vertices.size(); ++i) {
    vertices_geo[i].Pos = mesh_shpere.Vertices[i].Position;
    vertices_geo[i].Normal =  mesh_shpere.Vertices[i].Normal;
    vertices_geo[i].TexCoord =  mesh_shpere.Vertices[i].TexC;
    vertices_geo[i].TangentU = mesh_shpere.Vertices[i].TangentU;
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

  geo->DrawArgs["Sphere"] = std::move(box_sub_mesh_geo);
  //geo->DrawArgs["Sphere"] = std::move(sphere_sub_mesh_geo);
  //
  mesh_geos_[geo->Name] = std::move(geo);

  //  ------------------------
  auto mesh_quan = geo_generator.CreateQuad(0.0f, 0.0f, 1.0f, 1.0f, 0.0f);
  //  auto mesh_quan = geo_generator.CreateSphere(1.0f, 20, 20); //

  vector<Vertex> vertices_quan(mesh_quan.Vertices.size());
  vector<uint16_t> indices_quan = mesh_quan.GetIndices16();
  for (int i = 0; i < mesh_quan.Vertices.size(); ++i) {
    vertices_quan[i].Pos = mesh_quan.Vertices[i].Position;
    vertices_quan[i].Normal = mesh_quan.Vertices[i].Normal;
    vertices_quan[i].TexCoord = mesh_quan.Vertices[i].TexC;
    vertices_quan[i].TangentU = mesh_quan.Vertices[i].TangentU;
  }

  vertex_byte_size = (UINT)vertices_quan.size() * sizeof(Vertex);
  index_byte_size = (UINT)indices_quan.size() * sizeof(std::uint16_t);


  auto geo_quan = make_unique<MeshGeometry>();
  geo_quan->Name = "QuanGeo";

  ThrowIfFailed(D3DCreateBlob(vertex_byte_size, geo_quan->VertexBufferCpu.GetAddressOf()));
  CopyMemory(geo_quan->VertexBufferCpu->GetBufferPointer(), vertices_quan.data(), vertex_byte_size);

  geo_quan->VertexBufferGpu = d3dUtil::CreateDefaultBuffer(d3d_device_.Get(), d3d_command_list_.Get(),
    vertices_quan.data(), vertex_byte_size, geo_quan->VertexBufferUpload);

  ThrowIfFailed(D3DCreateBlob(index_byte_size, geo_quan->IndexBufferCpu.GetAddressOf()));
  CopyMemory(geo_quan->IndexBufferCpu->GetBufferPointer(), indices_quan.data(), index_byte_size);

  geo_quan->IndexBufferGpu = d3dUtil::CreateDefaultBuffer(d3d_device_.Get(), d3d_command_list_.Get(),
    indices_quan.data(), index_byte_size, geo_quan->IndexBufferUpload);

  geo_quan->VertexBufferSize = vertex_byte_size;
  geo_quan->VertexByteStride = (UINT)sizeof(Vertex);

  geo_quan->IndexBufferSize = index_byte_size;

  SubmeshGeometry quan_sub_mesh_geo;
  quan_sub_mesh_geo.IndexCount = (UINT)indices_quan.size();
  quan_sub_mesh_geo.BaseVertexLocation = 0;
  quan_sub_mesh_geo.StartIndexLocation = 0;

  geo_quan->DrawArgs["Quan"] = std::move(quan_sub_mesh_geo);

  mesh_geos_[geo_quan->Name] = std::move(geo_quan);

  //  ---------------
  auto mesh_grid = geo_generator.CreateGrid(20.0f, 30.0f, 60, 40);; //  CreateQuad(0.0f, 0.0f, 1.0f, 1.0f, 0.0f);

  vector<Vertex> vertices_grid(mesh_grid.Vertices.size());
  vector<uint16_t> indices_grid = mesh_grid.GetIndices16();
  for (int i = 0; i < mesh_grid.Vertices.size(); ++i) {
    vertices_grid[i].Pos = mesh_grid.Vertices[i].Position;
    vertices_grid[i].Normal = mesh_grid.Vertices[i].Normal;
    vertices_grid[i].TexCoord = mesh_grid.Vertices[i].TexC;
    vertices_grid[i].TangentU = mesh_grid.Vertices[i].TangentU;
  }

  vertex_byte_size = (UINT)vertices_grid.size() * sizeof(Vertex);
  index_byte_size = (UINT)indices_grid.size() * sizeof(std::uint16_t);


  auto geo_grid = make_unique<MeshGeometry>();
  geo_grid->Name = "GridGeo";

  ThrowIfFailed(D3DCreateBlob(vertex_byte_size, geo_grid->VertexBufferCpu.GetAddressOf()));
  CopyMemory(geo_grid->VertexBufferCpu->GetBufferPointer(), vertices_grid.data(), vertex_byte_size);

  geo_grid->VertexBufferGpu = d3dUtil::CreateDefaultBuffer(d3d_device_.Get(), d3d_command_list_.Get(),
    vertices_grid.data(), vertex_byte_size, geo_grid->VertexBufferUpload);

  ThrowIfFailed(D3DCreateBlob(index_byte_size, geo_grid->IndexBufferCpu.GetAddressOf()));
  CopyMemory(geo_grid->IndexBufferCpu->GetBufferPointer(), indices_grid.data(), index_byte_size);

  geo_grid->IndexBufferGpu = d3dUtil::CreateDefaultBuffer(d3d_device_.Get(), d3d_command_list_.Get(),
    indices_grid.data(), index_byte_size, geo_grid->IndexBufferUpload);

  geo_grid->VertexBufferSize = vertex_byte_size;
  geo_grid->VertexByteStride = (UINT)sizeof(Vertex);

  geo_grid->IndexBufferSize = index_byte_size;

  SubmeshGeometry grid_sub_mesh_geo;
  grid_sub_mesh_geo.IndexCount = (UINT)indices_grid.size();
  grid_sub_mesh_geo.BaseVertexLocation = 0;
  grid_sub_mesh_geo.StartIndexLocation = 0;

  geo_grid->DrawArgs["Grid"] = std::move(grid_sub_mesh_geo);

  mesh_geos_[geo_grid->Name] = std::move(geo_grid);

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

  XMFLOAT3 v_min_f3 = XMFLOAT3(+MathHelper::Infinity, +MathHelper::Infinity, +MathHelper::Infinity);
  XMFLOAT3 v_max_f3 = XMFLOAT3(-MathHelper::Infinity, -MathHelper::Infinity, -MathHelper::Infinity);

  XMVECTOR v_min = XMLoadFloat3(&v_min_f3);
  XMVECTOR v_max = XMLoadFloat3(&v_max_f3);

  for (UINT i=0; i < vertex_count; ++i) {
    fin >> vertices_geo[i].Pos.x >> vertices_geo[i].Pos.y >> vertices_geo[i].Pos.z;
    fin >> vertices_geo[i].Normal.x >> vertices_geo[i].Normal.y >> vertices_geo[i].Normal.z;
    

    XMVECTOR pos = XMLoadFloat3(&vertices_geo[i].Pos);

    XMFLOAT3 spherePos;
		XMStoreFloat3(&spherePos, XMVector3Normalize(pos));

		float theta = atan2f(spherePos.z, spherePos.x);

		// Put in [0, 2pi].
		if(theta < 0.0f)
			theta += XM_2PI;

		float phi = acosf(spherePos.y);

		float u = theta / (2.0f*XM_PI);
		float v = phi / XM_PI;

    //  vertices_geo[i].TexCoord = { 0.0f, 0.0f };
    vertices_geo[i].TexCoord = { u, v };

    v_min = XMVectorMin(v_min, pos);  //  find the min point
    v_max = XMVectorMax(v_max, pos);  //  find the max point
  }

    BoundingBox bounds;
    XMStoreFloat3(&bounds.Center, 0.5f * (v_min + v_max));
    XMStoreFloat3(&bounds.Extents, 0.5f * (v_max - v_min));


  //BoundingSphere bounds;

  //XMStoreFloat3(&bounds.Center, 0.5f * (v_min + v_max));
  //XMFLOAT3 d;
  //XMStoreFloat3(&d, 0.5f * (v_max - v_min ));
  //float ratio = 0.5f;
  //bounds.Radius = ratio * sqrtf(d.x * d.x + d.y * d.y + d.z * d.z);


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

  skull_sub_mesh_geo.Bounds = bounds;

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


  //  ------------

 // auto floorRitem = std::make_unique<RenderItem>();
	//floorRitem->World = MathHelper::Identity4x4();
	//floorRitem->TexTransform = MathHelper::Identity4x4();
	//floorRitem->ObjectCBIndex = 0;
	//floorRitem->Mat = materials_["checkertile"].get();
	//floorRitem->Geo = mesh_geos_["roomGeo"].get();
	//floorRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	//floorRitem->IndexCount = floorRitem->Geo->DrawArgs["floor"].IndexCount;
	//floorRitem->StartIndexLocation = floorRitem->Geo->DrawArgs["floor"].StartIndexLocation;
	//floorRitem->BaseVertexLocation = floorRitem->Geo->DrawArgs["floor"].BaseVertexLocation;
	//items_layers_[(int)RenderLayer::kOpaque].push_back(floorRitem.get());

 // auto wallsRitem = std::make_unique<RenderItem>();
	//wallsRitem->World = MathHelper::Identity4x4();
	//wallsRitem->TexTransform = MathHelper::Identity4x4();
	//wallsRitem->ObjectCBIndex = 1;
	//wallsRitem->Mat = materials_["bricks"].get();
	//wallsRitem->Geo = mesh_geos_["roomGeo"].get();
	//wallsRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	//wallsRitem->IndexCount = wallsRitem->Geo->DrawArgs["wall"].IndexCount;
	//wallsRitem->StartIndexLocation = wallsRitem->Geo->DrawArgs["wall"].StartIndexLocation;
	//wallsRitem->BaseVertexLocation = wallsRitem->Geo->DrawArgs["wall"].BaseVertexLocation;
	//items_layers_[(int)RenderLayer::kOpaque].push_back(wallsRitem.get());


 // XMMATRIX skullRotate = XMMatrixRotationY(0.5f*MathHelper::Pi);
	//XMMATRIX skullScale = XMMatrixScaling(0.45f, 0.45f, 0.45f);
	//XMMATRIX skullOffset = XMMatrixTranslation(-1.0f, 0.0f, -4.0f);
 // XMMATRIX skull_world = skullRotate * skullScale * skullOffset;

	//auto skullRitem = std::make_unique<RenderItem>();
	////  skullRitem->World = MathHelper::Identity4x4();

 // XMStoreFloat4x4(&skullRitem->World, skull_world);
	//skullRitem->TexTransform = MathHelper::Identity4x4();
	//skullRitem->ObjectCBIndex = 2;
	//skullRitem->Mat = materials_["bricks"].get();
	//skullRitem->Geo = mesh_geos_["skullGeo"].get();
	//skullRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	//skullRitem->IndexCount = skullRitem->Geo->DrawArgs["skull"].IndexCount;
	//skullRitem->StartIndexLocation = skullRitem->Geo->DrawArgs["skull"].StartIndexLocation;
	//skullRitem->BaseVertexLocation = skullRitem->Geo->DrawArgs["skull"].BaseVertexLocation;
	//mSkullRitem = skullRitem.get();
	//items_layers_[(int)RenderLayer::kOpaque].push_back(skullRitem.get());

	//// Reflected skull will have different world matrix, so it needs to be its own render item.
 // // Update reflection world matrix.
	//XMVECTOR mirrorPlane = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f); // xy plane
	//XMMATRIX R = XMMatrixReflect(mirrorPlane);
	//
	//auto reflectedSkullRitem = std::make_unique<RenderItem>();
	//*reflectedSkullRitem = *skullRitem;
 // XMStoreFloat4x4(&reflectedSkullRitem->World, skull_world * R);
 // //  XMStoreFloat4x4(&skullRitem->World, XMMatrixScaling(0.5f,0.5f,0.5f) * XMMatrixRotationAxis({0.0f, 1.0f, 0.0f}, 90.0f) * XMMatrixTranslation(-1.0f, 0.0f, 4.0f));
	//reflectedSkullRitem->ObjectCBIndex = 3;
	//mReflectedSkullRitem = reflectedSkullRitem.get();
	//items_layers_[(int)RenderLayer::kReflected].push_back(reflectedSkullRitem.get());

	//// Shadowed skull will have different world matrix, so it needs to be its own render item.
	//auto shadowedSkullRitem = std::make_unique<RenderItem>();
	//*shadowedSkullRitem = *skullRitem;
	//shadowedSkullRitem->ObjectCBIndex = 4;
	//shadowedSkullRitem->Mat = materials_["shadowMat"].get();
	//mShadowedSkullRitem = shadowedSkullRitem.get();
	//items_layers_[(int)RenderLayer::kOpaque].push_back(shadowedSkullRitem.get());

	//auto mirrorRitem = std::make_unique<RenderItem>();
	//mirrorRitem->World = MathHelper::Identity4x4();
	//mirrorRitem->TexTransform = MathHelper::Identity4x4();
	//mirrorRitem->ObjectCBIndex = 5;
	//mirrorRitem->Mat = materials_["icemirror"].get();
	//mirrorRitem->Geo = mesh_geos_["roomGeo"].get();
	//mirrorRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	//mirrorRitem->IndexCount = mirrorRitem->Geo->DrawArgs["mirror"].IndexCount;
	//mirrorRitem->StartIndexLocation = mirrorRitem->Geo->DrawArgs["mirror"].StartIndexLocation;
	//mirrorRitem->BaseVertexLocation = mirrorRitem->Geo->DrawArgs["mirror"].BaseVertexLocation;
	//items_layers_[(int)RenderLayer::kMirrors].push_back(mirrorRitem.get());
	//items_layers_[(int)RenderLayer::kTransparent].push_back(mirrorRitem.get());

	//render_items_.push_back(std::move(floorRitem));
	//render_items_.push_back(std::move(wallsRitem));
	//render_items_.push_back(std::move(skullRitem));
	//render_items_.push_back(std::move(reflectedSkullRitem));
	//render_items_.push_back(std::move(shadowedSkullRitem));
	//render_items_.push_back(std::move(mirrorRitem));

  

  //  --------------

  

  //render_items_.push_back(std::move(skull_render_item));

 // XMMATRIX skullRotate = XMMatrixRotationY(0.5f*MathHelper::Pi);
	//XMMATRIX skullScale = XMMatrixScaling(0.45f, 0.45f, 0.45f);
	//XMMATRIX skullOffset = XMMatrixTranslation(-1.0f, 0.0f, -4.0f);
 // XMMATRIX skull_world = skullRotate * skullScale * skullOffset;

  auto skullRitem = std::make_unique<RenderItem>();
  //  XMStoreFloat4x4(&skullRitem->World, skull_world);
  skullRitem->World = MathHelper::Identity4x4();
	skullRitem->TexTransform = MathHelper::Identity4x4();
	skullRitem->ObjectCBIndex = 0;
	//  skullRitem->Mat = materials_["skullMat"].get();
  //  skullRitem->Mat = materials_["grass"].get();
  skullRitem->Mat = MaterialManager::GetInstance()->GetMaterial("grass");
  
	skullRitem->Geo = mesh_geos_["skullGeo"].get();
	skullRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	skullRitem->IndexCount = skullRitem->Geo->DrawArgs["skull"].IndexCount;
	skullRitem->StartIndexLocation = skullRitem->Geo->DrawArgs["skull"].StartIndexLocation;
	skullRitem->BaseVertexLocation = skullRitem->Geo->DrawArgs["skull"].BaseVertexLocation;
  skullRitem->Bounds = skullRitem->Geo->DrawArgs["skull"].Bounds;

	skull_render_item_ = skullRitem.get();

  // Generate instance data.
	const int n = 5;
	instance_count_ = n*n*n;
	skullRitem->Instances.resize(instance_count_);


	//float width = 200.0f;
	//float height = 200.0f;
	//float depth = 200.0f;

	//float x = -0.5f*width;
	//float y = -0.5f*height;
	//float z = -0.5f*depth;
	//float dx = width / (n - 1);
	//float dy = height / (n - 1);
	//float dz = depth / (n - 1);
	//for(int k = 0; k < n; ++k)
	//{
	//	for(int i = 0; i < n; ++i)
	//	{
	//		for(int j = 0; j < n; ++j)
	//		{
	//			int index = k*n*n + i*n + j;
	//			// Position instanced along a 3D grid.
	//			skullRitem->Instances[index].World = XMFLOAT4X4(
	//				1.0f, 0.0f, 0.0f, 0.0f,
	//				0.0f, 1.0f, 0.0f, 0.0f,
	//				0.0f, 0.0f, 1.0f, 0.0f,
	//				x + j*dx, y + i*dy, z + k*dz, 1.0f);

	//			XMStoreFloat4x4(&skullRitem->Instances[index].TexTransform, XMMatrixScaling(2.0f, 2.0f, 1.0f));
	//			//  skullRitem->Instances[index].MaterialIndex = index % materials_.size();

 //       skullRitem->Instances[index].MaterialIndex = materials_["icemirror"]->MatCBIndex;
	//		}
	//	}
	//}

  int instance_count = 1;
  skullRitem->Instances.resize(instance_count);

  for (int i = 0; i < instance_count; ++i) {
    XMMATRIX rotate = XMMatrixRotationY(0.0f * MathHelper::Pi);
    XMMATRIX scale = XMMatrixScaling(1.0f, 1.0f, 1.0f);
    XMMATRIX offset = XMMatrixTranslation(10.0f * (i+1), 0.0f, -0.0f);
    XMMATRIX world = rotate * scale * offset;
    XMStoreFloat4x4(&skullRitem->Instances[i].World, world);
    skullRitem->Instances[i].MaterialIndex = MaterialManager::GetInstance()->GetMaterial("grass")->MatCBIndex; 
    //  materials_["skullMat"]->MatCBIndex;
  }

	items_layers_[(int)RenderLayer::kOpaque].push_back(skullRitem.get());

  auto sky_render_item = std::make_unique<RenderItem>();
  sky_render_item->Geo = mesh_geos_["ShapeGeo"].get();
  sky_render_item->Mat = MaterialManager::GetInstance()->GetMaterial("sky"); // materials_["sky"].get();
  sky_render_item->ObjectCBIndex = 0;

  //  XMStoreFloat4x4(&box_render_item->World, XMMatrixScaling(1.0f, 1.0f, 1.0f) * XMMatrixTranslation(-5.0f, 0.0f, 0.0f));
  sky_render_item->IndexCount = sky_render_item->Geo->DrawArgs["Sphere"].IndexCount;
  sky_render_item->PrimitiveType = D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
  sky_render_item->StartIndexLocation = sky_render_item->Geo->DrawArgs["Sphere"].StartIndexLocation;
  sky_render_item->BaseVertexLocation = sky_render_item->Geo->DrawArgs["Sphere"].BaseVertexLocation;
  sky_render_item->Instances.resize(1);
  XMStoreFloat4x4(&sky_render_item->Instances[0].World, XMMatrixScaling(10.0f, 10.0f, 10.0f)* XMMatrixTranslation(0.0f, 0.0f, 0.0f));

  //  sky_render_item->Instances[0].World = MathHelper::Identity4x4();
  //  pick_item->Instances[0].World = MathHelper::Identity4x4();
  sky_render_item->Instances[0].MaterialIndex = sky_render_item->Mat->MatCBIndex;
  sky_render_item->Visible = true;

  items_layers_[(int)RenderLayer::kSky].push_back(sky_render_item.get());

  auto ball_render_item = std::make_unique<RenderItem>();
  ball_render_item->Geo = mesh_geos_["ShapeGeo"].get();
  ball_render_item->Mat = MaterialManager::GetInstance()->GetMaterial("mirror0"); //  materials_[""].get();
  ball_render_item->ObjectCBIndex = 0;

  //  XMStoreFloat4x4(&box_render_item->World, XMMatrixScaling(1.0f, 1.0f, 1.0f) * XMMatrixTranslation(-5.0f, 0.0f, 0.0f));
  ball_render_item->IndexCount = ball_render_item->Geo->DrawArgs["Sphere"].IndexCount;
  ball_render_item->PrimitiveType = D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
  ball_render_item->StartIndexLocation = ball_render_item->Geo->DrawArgs["Sphere"].StartIndexLocation;
  ball_render_item->BaseVertexLocation = ball_render_item->Geo->DrawArgs["Sphere"].BaseVertexLocation;
  ball_render_item->Instances.resize(1);
  XMStoreFloat4x4(&ball_render_item->Instances[0].World, XMMatrixScaling(10.0f, 10.0f, 10.0f)* XMMatrixTranslation(0.0f, 0.0f, 0.0f));

  ball_render_item->Instances[0].MaterialIndex = ball_render_item->Mat->MatCBIndex;

  items_layers_[(int)RenderLayer::kReflected].push_back(ball_render_item.get());

  auto ball2_render_item = std::make_unique<RenderItem>();
  ball2_render_item->Geo = mesh_geos_["ShapeGeo"].get();
  ball2_render_item->Mat = MaterialManager::GetInstance()->GetMaterial("tile0"); 
  ball2_render_item->ObjectCBIndex = 0;

  //  XMStoreFloat4x4(&box_render_item->World, XMMatrixScaling(1.0f, 1.0f, 1.0f) * XMMatrixTranslation(-5.0f, 0.0f, 0.0f));
  ball2_render_item->IndexCount = ball2_render_item->Geo->DrawArgs["Sphere"].IndexCount;
  ball2_render_item->PrimitiveType = D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
  ball2_render_item->StartIndexLocation = ball2_render_item->Geo->DrawArgs["Sphere"].StartIndexLocation;
  ball2_render_item->BaseVertexLocation = ball2_render_item->Geo->DrawArgs["Sphere"].BaseVertexLocation;
  ball2_render_item->Instances.resize(1);
  XMStoreFloat4x4(&ball2_render_item->Instances[0].World, XMMatrixScaling(10.0f, 10.0f, 10.0f)* XMMatrixTranslation(0.0f, 15.0f, 0.0f));

  ball2_render_item->Instances[0].MaterialIndex = ball2_render_item->Mat->MatCBIndex;

  items_layers_[(int)RenderLayer::kMirrors].push_back(ball2_render_item.get());

  //  --------------
  auto quadRitem = std::make_unique<RenderItem>();
  quadRitem->World = MathHelper::Identity4x4();
  quadRitem->TexTransform = MathHelper::Identity4x4();
  quadRitem->Mat = MaterialManager::GetInstance()->GetMaterial("tile0");  //  mMaterials["bricks0"].get();
  quadRitem->Geo = mesh_geos_["QuanGeo"].get(); //mesh_geos_["skullGeo"].get();  //
  quadRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
  quadRitem->ObjectCBIndex = 0;
  quadRitem->IndexCount = quadRitem->Geo->DrawArgs["Quan"].IndexCount;
  quadRitem->StartIndexLocation = quadRitem->Geo->DrawArgs["Quan"].StartIndexLocation;
  quadRitem->BaseVertexLocation = quadRitem->Geo->DrawArgs["Quan"].BaseVertexLocation;
  //quadRitem->IndexCount = quadRitem->Geo->DrawArgs["skull"].IndexCount;
  //quadRitem->StartIndexLocation = quadRitem->Geo->DrawArgs["skull"].StartIndexLocation;
  //quadRitem->BaseVertexLocation = quadRitem->Geo->DrawArgs["skull"].BaseVertexLocation;

  quadRitem->Instances.resize(1);
  quadRitem->Instances[0].World = MathHelper::Identity4x4();
  quadRitem->Instances[0].MaterialIndex = quadRitem->Mat->MatCBIndex;
  quadRitem->Visible = true;

  items_layers_[(int)RenderLayer::kDebug].push_back(quadRitem.get());
  //  ------------

  auto gridRitem = std::make_unique<RenderItem>();
  gridRitem->World = MathHelper::Identity4x4();
  gridRitem->TexTransform = MathHelper::Identity4x4();
  gridRitem->Mat = MaterialManager::GetInstance()->GetMaterial("tile0");  //  mMaterials["bricks0"].get();
  gridRitem->Geo = mesh_geos_["GridGeo"].get(); //mesh_geos_["skullGeo"].get();  //
  gridRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
  gridRitem->ObjectCBIndex = 0;
  gridRitem->IndexCount = gridRitem->Geo->DrawArgs["Grid"].IndexCount;
  gridRitem->StartIndexLocation = gridRitem->Geo->DrawArgs["Grid"].StartIndexLocation;
  gridRitem->BaseVertexLocation = gridRitem->Geo->DrawArgs["Grid"].BaseVertexLocation;

  gridRitem->Instances.resize(1);
  gridRitem->Instances[0].World = MathHelper::Identity4x4();
  XMStoreFloat4x4(&gridRitem->Instances[0].World, XMMatrixScaling(10.0f, 10.0f, 10.0f)* XMMatrixTranslation(0.0f, -15.0f, 0.0f));
  gridRitem->Instances[0].MaterialIndex = gridRitem->Mat->MatCBIndex;
  gridRitem->Visible = true;

  items_layers_[(int)RenderLayer::kShadow].push_back(gridRitem.get());

  //  -----------
  

  XMMATRIX pick_rotate = XMMatrixRotationY(0.0f * MathHelper::Pi);
  XMMATRIX pick_scale = XMMatrixScaling(1.0f, 1.0f, 1.0f);
  XMMATRIX pick_offset = XMMatrixTranslation(-7.0f, 0.0f, -0.0f);
  XMMATRIX pick_world = pick_rotate * pick_scale * pick_offset;
  auto pick_item = std::make_unique<RenderItem>();
  //  pick_item->World = MathHelper::Identity4x4();
  XMStoreFloat4x4(&pick_item->World, pick_world);
  pick_item->TexTransform = MathHelper::Identity4x4();
  pick_item->ObjectCBIndex = 0;
  //  pick_item->Mat = materials_["skullMat"].get();
  pick_item->Mat = MaterialManager::GetInstance()->GetMaterial("icemirror"); // materials_["icemirror"].get();

  pick_item->Geo = mesh_geos_["skullGeo"].get();
  pick_item->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
  pick_item->IndexCount = pick_item->Geo->DrawArgs["skull"].IndexCount;
  pick_item->StartIndexLocation = pick_item->Geo->DrawArgs["skull"].StartIndexLocation;
  pick_item->BaseVertexLocation = pick_item->Geo->DrawArgs["skull"].BaseVertexLocation;

  pick_item->Instances.resize(1);
  XMStoreFloat4x4(&pick_item->Instances[0].World, pick_world);
  //  pick_item->Instances[0].World = MathHelper::Identity4x4();
  pick_item->Instances[0].MaterialIndex = pick_item->Mat->MatCBIndex;

  picked_item_ = pick_item.get();
  items_layers_[(int)RenderLayer::kTransparent].push_back(pick_item.get());

  render_items_.push_back(std::move(skullRitem));
  render_items_.push_back(std::move(sky_render_item));
  render_items_.push_back(std::move(ball2_render_item));
  render_items_.push_back(std::move(ball_render_item));
  render_items_.push_back(std::move(pick_item));
  //mRitemLayer[(int)RenderLayer::Debug].push_back(quadRitem.get());
  render_items_.push_back(std::move(quadRitem));
  render_items_.push_back(std::move(gridRitem));

}


void RenderSystem::BuildShadersAndInputLayout() {
  shaders_["standardVS"] = d3dUtil::CompileShader(L"Shaders\\default.hlsl", nullptr, "VS", "vs_5_1");
	shaders_["opaquePS"] = d3dUtil::CompileShader(L"Shaders\\default.hlsl", nullptr, "PS", "ps_5_1");
  shaders_["standardNormalVS"] = d3dUtil::CompileShader(L"Shaders\\StandardNormal.hlsl", nullptr, "VS", "vs_5_1");
  shaders_["standardNormalPS"] = d3dUtil::CompileShader(L"Shaders\\StandardNormal.hlsl", nullptr, "PS", "ps_5_1");
  shaders_["shadowVS"] = d3dUtil::CompileShader(L"Shaders\\Shadow.hlsl", nullptr, "VS", "vs_5_1");
  shaders_["shadowOpaquePS"] = d3dUtil::CompileShader(L"Shaders\\Shadow.hlsl", nullptr, "PS", "ps_5_1");

  shaders_["shadowDebugVS"] = d3dUtil::CompileShader(L"Shaders\\ShadowDebug.hlsl", nullptr, "VS", "vs_5_1");
  shaders_["shadowDebugPS"] = d3dUtil::CompileShader(L"Shaders\\ShadowDebug.hlsl", nullptr, "PS", "ps_5_1");

  shaders_["horzBlurCS"] = d3dUtil::CompileShader(L"Shaders\\Blur.hlsl", nullptr, "HorzBlurCS", "cs_5_0");
	shaders_["vertBlurCS"] = d3dUtil::CompileShader(L"Shaders\\Blur.hlsl", nullptr, "VertBlurCS", "cs_5_0");

  shaders_["skyVS"] = d3dUtil::CompileShader(L"Shaders\\Sky.hlsl", nullptr, "SkyVS", "vs_5_1");
  shaders_["skyPS"] = d3dUtil::CompileShader(L"Shaders\\Sky.hlsl", nullptr, "SkyPS", "ps_5_1");
	
  input_layout_ =
  {
      { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
      { "NORMAL", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
      { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
      { "TANGENT", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
  };
}

void RenderSystem::BuildRootSignature() {
  

  CD3DX12_DESCRIPTOR_RANGE tex_table1;
  tex_table1.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 0, 0);

  //CD3DX12_DESCRIPTOR_RANGE tex_table2;
  //tex_table2.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, 0);

  CD3DX12_DESCRIPTOR_RANGE tex_table3;
  tex_table3.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 7, 2, 0);

  //  CD3DX12_DESCRIPTOR_RANGE tex_table2;
  //  tex_table2.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 6, 0, 0);
  const int parameter_count = 5;

  CD3DX12_ROOT_PARAMETER slotRootParameter[parameter_count];
  
  slotRootParameter[0].InitAsShaderResourceView(0, 1);
  slotRootParameter[1].InitAsShaderResourceView(1, 1);
  slotRootParameter[2].InitAsConstantBufferView(0);
  slotRootParameter[3].InitAsDescriptorTable(1, &tex_table1, D3D12_SHADER_VISIBILITY_PIXEL);
  //  slotRootParameter[4].InitAsDescriptorTable(1, &tex_table2, D3D12_SHADER_VISIBILITY_PIXEL);
  slotRootParameter[4].InitAsDescriptorTable(1, &tex_table3, D3D12_SHADER_VISIBILITY_PIXEL);

  //  slotRootParameter[3].InitAsDescriptorTable(1, &tex_table2, D3D12_SHADER_VISIBILITY_PIXEL);

  auto static_samplers = GetStaticSamplers();

  CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(parameter_count, slotRootParameter,
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



void RenderSystem::BuildPSOs() {
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
  ThrowIfFailed(d3d_device_->CreateGraphicsPipelineState(&pso_desc, 
    IID_PPV_ARGS(&psos_["opaque"])));


  D3D12_GRAPHICS_PIPELINE_STATE_DESC shadowPsoDesc = pso_desc;
  shadowPsoDesc.RasterizerState.DepthBias = 100000;
  shadowPsoDesc.RasterizerState.DepthBiasClamp = 0.0f;
  shadowPsoDesc.RasterizerState.SlopeScaledDepthBias = 1.0f;
  shadowPsoDesc.pRootSignature = root_signature_.Get();
  shadowPsoDesc.VS =
  {
      reinterpret_cast<BYTE*>(shaders_["shadowVS"]->GetBufferPointer()),
      shaders_["shadowVS"]->GetBufferSize()
  };
  shadowPsoDesc.PS =
  {
      reinterpret_cast<BYTE*>(shaders_["shadowOpaquePS"]->GetBufferPointer()),
      shaders_["shadowOpaquePS"]->GetBufferSize()
  };

  // Shadow map pass does not have a render target.
  shadowPsoDesc.RTVFormats[0] = DXGI_FORMAT_UNKNOWN;
  shadowPsoDesc.NumRenderTargets = 0;
  ThrowIfFailed(d3d_device_->CreateGraphicsPipelineState(&shadowPsoDesc,
    IID_PPV_ARGS(&psos_["shadow_opaque"])));


  D3D12_GRAPHICS_PIPELINE_STATE_DESC standardNormalPsoDesc = pso_desc; 
  //standardNormalPsoDesc.VS =
  //{
  //  reinterpret_cast<BYTE*>(shaders_["standardNormalVS"]->GetBufferPointer()),
  //  shaders_["standardNormalVS"]->GetBufferSize()
  //};
  standardNormalPsoDesc.PS =
  {
    reinterpret_cast<BYTE*>(shaders_["standardNormalPS"]->GetBufferPointer()),
    shaders_["standardNormalPS"]->GetBufferSize()
  };
  ThrowIfFailed(d3d_device_->CreateGraphicsPipelineState(&standardNormalPsoDesc,
    IID_PPV_ARGS(&psos_["opaqueNormal"])));

  //  ----------

  D3D12_GRAPHICS_PIPELINE_STATE_DESC debugPsoDesc = pso_desc;
  debugPsoDesc.pRootSignature = root_signature_.Get();
  debugPsoDesc.VS =
  {
      reinterpret_cast<BYTE*>(shaders_["shadowDebugVS"]->GetBufferPointer()),
      shaders_["shadowDebugVS"]->GetBufferSize()
  };
  debugPsoDesc.PS =
  {
      reinterpret_cast<BYTE*>(shaders_["shadowDebugPS"]->GetBufferPointer()),
      shaders_["shadowDebugPS"]->GetBufferSize()
  };
  ThrowIfFailed(d3d_device_->CreateGraphicsPipelineState(&debugPsoDesc, IID_PPV_ARGS(&psos_["debug"])));

  //  -----------

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

  ThrowIfFailed(d3d_device_->CreateGraphicsPipelineState(&transparentPsoDesc, 
    IID_PPV_ARGS(&psos_[pso_name_transparent])));

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
  ThrowIfFailed(d3d_device_->CreateGraphicsPipelineState(&mirrorPsoDesc, IID_PPV_ARGS(&psos_[pso_name_mirror])));


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
  ThrowIfFailed(d3d_device_->CreateGraphicsPipelineState(&reflectionPsoDesc, 
    IID_PPV_ARGS(&psos_[pso_name_reflection])));




  D3D12_COMPUTE_PIPELINE_STATE_DESC horzBlurPSO = {};
	horzBlurPSO.pRootSignature = post_process_root_signature_.Get();
	horzBlurPSO.CS =
	{
		reinterpret_cast<BYTE*>(shaders_["horzBlurCS"]->GetBufferPointer()),
		shaders_["horzBlurCS"]->GetBufferSize()
	};
	horzBlurPSO.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	ThrowIfFailed(d3d_device_->CreateComputePipelineState(&horzBlurPSO, 
    IID_PPV_ARGS(&psos_["horzBlur"])));


  D3D12_COMPUTE_PIPELINE_STATE_DESC vertBlurPSO = {};
  vertBlurPSO.pRootSignature = post_process_root_signature_.Get();
  vertBlurPSO.CS = 
	{ 
    reinterpret_cast<BYTE*>(shaders_["vertBlurCS"]->GetBufferPointer()), 
		shaders_["vertBlurCS"]->GetBufferSize() 
	};
  vertBlurPSO.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
  ThrowIfFailed(d3d_device_->CreateComputePipelineState(&vertBlurPSO, 
    IID_PPV_ARGS(&psos_["vertBlur"])));

  D3D12_GRAPHICS_PIPELINE_STATE_DESC sky_pso_desc = pso_desc;
  sky_pso_desc.pRootSignature = root_signature_.Get();
  sky_pso_desc.VS = { 
    reinterpret_cast<BYTE*>(shaders_["skyVS"]->GetBufferPointer()),
    shaders_["skyVS"]->GetBufferSize() 
  };
  sky_pso_desc.PS = {
    reinterpret_cast<BYTE*>(shaders_["skyPS"]->GetBufferPointer()),
    shaders_["skyPS"]->GetBufferSize()
  };
  sky_pso_desc.RasterizerState.CullMode = D3D12_CULL_MODE::D3D12_CULL_MODE_NONE;
  sky_pso_desc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

  ThrowIfFailed(d3d_device_->CreateGraphicsPipelineState(&sky_pso_desc,
    IID_PPV_ARGS(&psos_["sky"])));

}

void RenderSystem::OnResize() {
  Application::OnResize();

  XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f*MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
    XMStoreFloat4x4(&mProj, P);

  if (blur_filter_ != nullptr) {
    blur_filter_->OnResize(client_width_, client_height_);
  }

  camera_.SetLens(0.25f*MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);

  BoundingFrustum::CreateFromMatrix(camera_frustum_, camera_.GetProj());
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

  if (GetAsyncKeyState('1') & 0x8000)
    enable_frustum_culling_ = true;

  if (GetAsyncKeyState('2') & 0x8000)
    enable_frustum_culling_ = false;

  if (GetAsyncKeyState('3') & 0x8000)
    use_4x_msaa_state_ = !use_4x_msaa_state_;

	camera_.UpdateViewMatrix();
}

void RenderSystem::OnMouseDown(WPARAM btnState, int x, int y) {
  if ((btnState & MK_LBUTTON) != 0) {
    last_mouse_pos.x = x;
    last_mouse_pos.y = y;

    SetCapture(h_window_);
  }
  else if ((btnState & MK_RBUTTON) != 0) {
    Pick(x, y);
    use_mouse_ = true;
    std::wostringstream outs;
    outs.precision(0);
    outs << "                                 (" << x << ", " << y << ")";
    wstring log = outs.str();
    //  main_wnd_caption_ = outs.str();

    SetWindowText(h_window_, log.c_str());
  }
}

void RenderSystem::OnMouseUp(WPARAM btnState, int x, int y) {
  ReleaseCapture();
  use_mouse_ = false;
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

void RenderSystem::LoadTexture() {
  std::map<string, wstring> texs = {
    {"bricksTex", L"Textures/bricks3.dds"},
    {"checkboardTex", L"Textures/checkboard.dds"},
    {"tileDiffuseMap", L"Textures/tile.dds"},
    {"tileNormalMap", L"Textures/tile_nmap.dds"},
    {"iceTex", L"Textures/ice.dds"},
    {"white1x1Tex", L"Textures/white1x1.dds"},
    {"grass", L"Textures/grass.dds"},
    {"skyCubeMap", L"Textures/grasscube1024.dds"},
    {"snowcube", L"Textures/snowcube1024.dds"}
  };

  for (auto& it : texs) {
    auto texture = std::make_unique<Texture>();
    texture->Name = it.first;
    texture->Filename = it.second;
    ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(d3d_device_.Get(),
      d3d_command_list_.Get(), texture->Filename.c_str(),
      texture->Resource, texture->UploadHeap));

    textures_[texture->Name] = std::move(texture);
  }
}

void RenderSystem::BuildDescriptorHeaps() {
  std::vector<Texture*> texture2d = {
    textures_["bricksTex"].get(),
    textures_["checkboardTex"].get(),
    textures_["iceTex"].get(),
    textures_["white1x1Tex"].get(),
    textures_["grass"].get(),
    textures_["tileDiffuseMap"].get(),
    textures_["tileNormalMap"].get(),
  };

  const int textureDescriptorCount = texture2d.size() + 2 + 6;

  D3D12_DESCRIPTOR_HEAP_DESC src_heap_desc = {};
  //  src_heap_desc.NumDescriptors = textureDescriptorCount + blurDescriptorCount;
  src_heap_desc.NumDescriptors = textureDescriptorCount;
  src_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

  src_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAGS::D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

  ThrowIfFailed(d3d_device_->CreateDescriptorHeap(&src_heap_desc, IID_PPV_ARGS(srv_descriptor_heap_.GetAddressOf())));

  CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(srv_descriptor_heap_->GetCPUDescriptorHandleForHeapStart());

  //auto wood_crate_tex = textures_["Wood"]->Resource;
  //auto grass_tex = textures_["Grass"]->Resource;
  //auto water1_tex = textures_["Water"]->Resource; 

  

  D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
  srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
  srv_desc.Texture2D.MostDetailedMip = 0;
  srv_desc.Texture2D.ResourceMinLODClamp = 0.0f;

  int offsetCount = 0;
  for (auto& texture : texture2d) {
    texture->SrvHeapIndex = offsetCount;

    auto desc = texture->Resource->GetDesc();

    srv_desc.Format = desc.Format;
    srv_desc.Texture2D.MipLevels = desc.MipLevels;
    d3d_device_->CreateShaderResourceView(texture->Resource.Get(), &srv_desc, hDescriptor);
    
    hDescriptor.Offset(1, cbv_srv_uav_descriptor_size);
    ++offsetCount;
  }

  auto skyTex = textures_["snowcube"]->Resource;
  srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
  srv_desc.TextureCube.MostDetailedMip = 0;
  srv_desc.TextureCube.MipLevels = skyTex->GetDesc().MipLevels;
  srv_desc.TextureCube.ResourceMinLODClamp = 0.0f;
  srv_desc.Format = skyTex->GetDesc().Format;
  d3d_device_->CreateShaderResourceView(skyTex.Get(), &srv_desc, hDescriptor);  //  5
  textures_["snowcube"]->SrvHeapIndex = texture2d.size();

  //blur_filter_->BuildDescriptors(
  //  CD3DX12_CPU_DESCRIPTOR_HANDLE(srv_descriptor_heap_->GetCPUDescriptorHandleForHeapStart(), 6, cbv_srv_uav_descriptor_size),
  //  CD3DX12_GPU_DESCRIPTOR_HANDLE(srv_descriptor_heap_->GetGPUDescriptorHandleForHeapStart(), 6, cbv_srv_uav_descriptor_size),
  //  cbv_srv_uav_descriptor_size);

  sky_heap_index_ = texture2d.size();
  //  dynamic_heap_index_ = 1 + sky_heap_index_;

  auto srv_cpu_start = srv_descriptor_heap_->GetCPUDescriptorHandleForHeapStart();
  auto srv_gpu_start = srv_descriptor_heap_->GetGPUDescriptorHandleForHeapStart();
  auto rtv_cpu_start = rtv_heap_->GetCPUDescriptorHandleForHeapStart();
  auto dsv_cpu_start = dsv_heap_->GetCPUDescriptorHandleForHeapStart();

  //int rtv_offset = kSwapBufferCount;

  //CD3DX12_CPU_DESCRIPTOR_HANDLE cubeRtvHandles[6];
  //for (int i = 0; i < 6; ++i)
  //  cubeRtvHandles[i] = CD3DX12_CPU_DESCRIPTOR_HANDLE(rtv_cpu_start, rtv_offset + i, rtv_descriptor_size);

  //CD3DX12_CPU_DESCRIPTOR_HANDLE(
  //  rtv_heap_->GetCPUDescriptorHandleForHeapStart(),
  //  current_back_buffer_index_,
  //  rtv_descriptor_size);

  //dynamic_cube_map_->BuildDescriptors(
  //  CD3DX12_CPU_DESCRIPTOR_HANDLE(srv_cpu_start, dynamic_heap_index_, cbv_srv_uav_descriptor_size),
  //  CD3DX12_GPU_DESCRIPTOR_HANDLE(srv_gpu_start, dynamic_heap_index_, cbv_srv_uav_descriptor_size),
  //  cubeRtvHandles);


  //shadow_heap_index = dynamic_heap_index_ + 1;
  shadow_heap_index = sky_heap_index_ + 1;
  int mNullCubeSrvIndex = shadow_heap_index + 1;
  int mNullTexSrvIndex = mNullCubeSrvIndex + 1;

  shadow_map_->BuildDescriptors(
    CD3DX12_CPU_DESCRIPTOR_HANDLE(srv_cpu_start, shadow_heap_index, cbv_srv_uav_descriptor_size),
    CD3DX12_GPU_DESCRIPTOR_HANDLE(srv_gpu_start, shadow_heap_index, cbv_srv_uav_descriptor_size),
    CD3DX12_CPU_DESCRIPTOR_HANDLE(dsv_cpu_start, 1, dsv_descriptor_size)
  );

  auto null_cube_srv_handle_cpu = CD3DX12_CPU_DESCRIPTOR_HANDLE(srv_cpu_start, mNullCubeSrvIndex, cbv_srv_uav_descriptor_size);
  null_cube_srv_handle_gpu = CD3DX12_GPU_DESCRIPTOR_HANDLE(srv_gpu_start, mNullCubeSrvIndex, cbv_srv_uav_descriptor_size);

  d3d_device_->CreateShaderResourceView(nullptr, &srv_desc, null_cube_srv_handle_cpu);

  null_cube_srv_handle_cpu.Offset(1, cbv_srv_uav_descriptor_size);

  srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
  srv_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  srv_desc.Texture2D.MostDetailedMip = 0;
  srv_desc.Texture2D.ResourceMinLODClamp = 0.0f;
  srv_desc.Texture2D.MipLevels = 1;
  d3d_device_->CreateShaderResourceView(nullptr, &srv_desc, null_cube_srv_handle_cpu);
  
}

void RenderSystem::BuildMaterial() {


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

  //  auto bricks = std::make_unique<Material>();
  auto bricks = MaterialManager::GetInstance()->CreateMaterial("bricks");
  //  bricks->Name = "bricks";
  //  bricks->MatCBIndex = 0;
  bricks->DiffuseSrvHeapIndex = textures_["bricksTex"]->SrvHeapIndex;
  bricks->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
  bricks->FresnelR0 = XMFLOAT3(0.05f, 0.05f, 0.05f);
  bricks->Roughness = 0.25f;

  auto checkertile = MaterialManager::GetInstance()->CreateMaterial("checkertile");
  //  auto checkertile = std::make_unique<Material>();
  //  checkertile->Name = "checkertile";
  //  checkertile->MatCBIndex = 1;
  checkertile->DiffuseSrvHeapIndex = textures_["checkboardTex"]->SrvHeapIndex;
  checkertile->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
  checkertile->FresnelR0 = XMFLOAT3(0.07f, 0.07f, 0.07f);
  checkertile->Roughness = 0.3f;

  //  auto icemirror = std::make_unique<Material>();
  auto icemirror = MaterialManager::GetInstance()->CreateMaterial("icemirror");
  //  icemirror->Name = "icemirror";
  //  icemirror->MatCBIndex = 2;
  icemirror->DiffuseSrvHeapIndex = textures_["iceTex"]->SrvHeapIndex;
  icemirror->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 0.3f);
  icemirror->FresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
  icemirror->Roughness = 0.5f;

  //  auto skullMat = std::make_unique<Material>();
  auto skullMat = MaterialManager::GetInstance()->CreateMaterial("skullMat");
  //  skullMat->Name = "skullMat";
  //  skullMat->MatCBIndex = 3;
  skullMat->DiffuseSrvHeapIndex = 3;
  skullMat->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
  skullMat->FresnelR0 = XMFLOAT3(0.05f, 0.05f, 0.05f);
  skullMat->Roughness = 0.3f;

  //  auto shadowMat = std::make_unique<Material>();
  auto shadowMat = MaterialManager::GetInstance()->CreateMaterial("shadowMat");
  //  shadowMat->Name = "shadowMat";
  //  shadowMat->MatCBIndex = 4;
  shadowMat->DiffuseSrvHeapIndex = 3;
  shadowMat->DiffuseAlbedo = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.5f);
  shadowMat->FresnelR0 = XMFLOAT3(0.001f, 0.001f, 0.001f);
  shadowMat->Roughness = 0.0f;

  //  auto mirror0 = std::make_unique<Material>();
  auto mirror0 = MaterialManager::GetInstance()->CreateMaterial("mirror0");
  //  mirror0->Name = "mirror0";
  //  mirror0->MatCBIndex = 5;
  mirror0->DiffuseSrvHeapIndex = 3;
  mirror0->DiffuseAlbedo = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
  mirror0->FresnelR0 = XMFLOAT3(0.98f, 0.97f, 0.95f);
  mirror0->Roughness = 0.1f;

  auto grass = MaterialManager::GetInstance()->CreateMaterial("grass");
  //  auto grass = std::make_unique<Material>();
  //  grass->Name = "grass";
  grass->MatCBIndex = 6;
  grass->DiffuseSrvHeapIndex = textures_["grass"]->SrvHeapIndex;
  //grass->DiffuseAlbedo = XMFLOAT4(0.2f, 0.6f, 0.2f, 1.0f);
  grass->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
  grass->FresnelR0 = XMFLOAT3(0.01f, 0.01f, 0.01f);
  grass->Roughness = 0.125f;

  //  auto tile0 = std::make_unique<Material>();
  auto tile0 = MaterialManager::GetInstance()->CreateMaterial("tile0");
  //  tile0->Name = "tile0";
  //  tile0->MatCBIndex = 7;
  tile0->DiffuseSrvHeapIndex = textures_["tileDiffuseMap"]->SrvHeapIndex;
  tile0->NormalSrvHeapIndex = textures_["tileNormalMap"]->SrvHeapIndex; 
  //  grass->DiffuseSrvHeapIndex = textures_["grass"]->SrvHeapIndex;
  tile0->DiffuseAlbedo = XMFLOAT4(0.9f, 0.9f, 0.9f, 1.0f);
  tile0->FresnelR0 = XMFLOAT3(0.2f, 0.2f, 0.2f);
  tile0->Roughness = 0.1f;

  auto sky = MaterialManager::GetInstance()->CreateMaterial("sky");
  //  auto sky = std::make_unique<Material>();
  //  sky->Name = "sky";
  sky->MatCBIndex = 8;
  sky->DiffuseSrvHeapIndex = textures_["skyCubeMap"]->SrvHeapIndex;
  sky->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
  sky->FresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
  sky->Roughness = 1.0f;

  //materials_["bricks"] = std::move(bricks);
  //materials_["checkertile"] = std::move(checkertile);
  //materials_["icemirror"] = std::move(icemirror);
  //materials_["skullMat"] = std::move(skullMat);
  //materials_["shadowMat"] = std::move(shadowMat);
  //materials_["mirror0"] = std::move(mirror0);
  //materials_["grass"] = std::move(grass);
  //materials_["tile0"] = std::move(tile0);
  //materials_["sky"] = std::move(sky);
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

std::array<const CD3DX12_STATIC_SAMPLER_DESC, SAMPLER_COUNT> RenderSystem::GetStaticSamplers()
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

void RenderSystem::Pick(int sx, int sy) {
  XMFLOAT4X4 proj = camera_.GetProj4x4f();

  float vx = (2.0f * sx / client_width_ - 1.0f) * proj(0, 0);
  float vy = (-2.0f * sy / client_height_ + 1.0f) * proj(1, 1);

  XMVECTOR ray_origin = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
  XMVECTOR ray_direction = XMVectorSet(vx, vy, 1.0f, 0.0f);

  XMMATRIX view = camera_.GetView();
  XMMATRIX inv_view = XMMatrixInverse(&XMMatrixDeterminant(view), view);

  XMVECTOR ray_origin_local = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
  XMVECTOR ray_direction_local = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);


  for (auto& ri : items_layers_[(int)RenderLayer::kOpaque]) {

    const auto instances = ri->Instances;

    //  for (auto& instance : instances) {
    for (int i = 0; i < instances.size(); ++i) {
      InstanceData instance = instances[i];
      XMMATRIX world = XMLoadFloat4x4(&instance.World);
      XMMATRIX inv_world = XMMatrixInverse(&XMMatrixDeterminant(world), world);

      XMMATRIX to_local = XMMatrixMultiply(inv_view, inv_world);

      ray_origin_local = XMVector3TransformCoord(ray_origin, to_local);
      ray_direction_local = XMVector3TransformNormal(ray_direction, to_local);
      ray_direction_local = XMVector3Normalize(ray_direction_local);

      float ray_distance = 1.0f;

      if (ri->Bounds.Intersects(ray_origin_local, ray_direction_local, ray_distance)) {
        if (picked_instance_ != nullptr) {
          picked_instance_->MaterialIndex = ri->Mat->MatCBIndex;
        }
        ri->Instances[i].MaterialIndex = picked_item_->Mat->MatCBIndex;
        picked_instance_ = &ri->Instances[i];

        return;
      }
    }

  }

}


void RenderSystem::BuildCubeFaceCamera(float x, float y, float z)
{
  // Generate the cube map about the given position.
  XMFLOAT3 center(x, y, z);
  XMFLOAT3 worldUp(0.0f, 1.0f, 0.0f);

  // Look along each coordinate axis.
  XMFLOAT3 targets[6] =
  {
    XMFLOAT3(x + 1.0f, y, z), // +X
    XMFLOAT3(x - 1.0f, y, z), // -X
    XMFLOAT3(x, y + 1.0f, z), // +Y
    XMFLOAT3(x, y - 1.0f, z), // -Y
    XMFLOAT3(x, y, z + 1.0f), // +Z
    XMFLOAT3(x, y, z - 1.0f)  // -Z
  };

  // Use world up vector (0,1,0) for all directions except +Y/-Y.  In these cases, we
  // are looking down +Y or -Y, so we need a different "up" vector.
  XMFLOAT3 ups[6] =
  {
    XMFLOAT3(0.0f, 1.0f, 0.0f),  // +X
    XMFLOAT3(0.0f, 1.0f, 0.0f),  // -X
    XMFLOAT3(0.0f, 0.0f, -1.0f), // +Y
    XMFLOAT3(0.0f, 0.0f, +1.0f), // -Y
    XMFLOAT3(0.0f, 1.0f, 0.0f),	 // +Z
    XMFLOAT3(0.0f, 1.0f, 0.0f)	 // -Z
  };

  for (int i = 0; i < 6; ++i)
  {
    cube_map_cameras_[i].LookAt(center, targets[i], ups[i]);
    cube_map_cameras_[i].SetLens(0.5f * XM_PI, 1.0f, 0.1f, 1000.0f);
    cube_map_cameras_[i].UpdateViewMatrix();
  }
}

}


