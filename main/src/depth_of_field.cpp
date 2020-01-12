#include "depth_of_field.h"
#include "graphics_core.h"
#include "command_context.h"
#include "camera.h"
#include "graphics_utility.h"
#include "game_setting.h"

namespace PostProcess {

using namespace GameCore;

DepthOfField DoF;

void DepthOfField::Initialize() {
  width_ = Graphics::Core.Width();
  height_ = Graphics::Core.Height();
  //Graphics::Core.BackBufferFormat

  auto format = Graphics::Core.BackBufferFormat;
  
  blur_buffer_.Create(L"DoF Blur Map 00", width_, height_, 1, format);
  color_buffer_.Create(L"DoF Blur Map 01", width_, height_, 1, format);
  depth_buffer_.Create(L"DoF DepthBuffer", width_, height_, Graphics::Core.DepthStencilFormat);

  blur_root_signature_.Reset(3, 0);
  blur_root_signature_[0].InitAsConstants(12, 0);
  blur_root_signature_[1].InitAsDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);  //  0, D3D12_SHADER_VISIBILITY_PIXEL
  blur_root_signature_[2].InitAsDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);
  blur_root_signature_.Finalize();

  auto horz_blur_cs = d3dUtil::CompileShader(L"shaders/blur.hlsl", nullptr, "HorzBlurCS", "cs_5_0");
  horizontal_pso_.SetRootSignature(blur_root_signature_);
  horizontal_pso_.SetComputeShader(reinterpret_cast<BYTE*>(horz_blur_cs->GetBufferPointer()), (UINT)horz_blur_cs->GetBufferSize());
  horizontal_pso_.Finalize();

  auto vert_blur_cs = d3dUtil::CompileShader(L"shaders/blur.hlsl", nullptr, "VertBlurCS", "cs_5_0");
  vertical_pso_.SetRootSignature(blur_root_signature_);
  vertical_pso_.SetComputeShader(reinterpret_cast<BYTE*>(vert_blur_cs->GetBufferPointer()), (UINT)vert_blur_cs->GetBufferSize());
  vertical_pso_.Finalize();

  //  --------------------
  dof_buffer_.Create(L"DoF Standard Map 00", width_, height_, 1, format);
  auto dof_cs = d3dUtil::CompileShader(L"shaders/dof.hlsl", nullptr, "DofCS", "cs_5_0");
  dof_root_signature_.Reset(3, 0);
  dof_root_signature_[0].InitAsConstants(4, 0);
  dof_root_signature_[1].InitAsDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 3, 0);  //  0, D3D12_SHADER_VISIBILITY_PIXEL
  dof_root_signature_[2].InitAsDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);
  dof_root_signature_.Finalize();

  dof_pso_.SetRootSignature(dof_root_signature_);
  dof_pso_.SetComputeShader(reinterpret_cast<BYTE*>(dof_cs->GetBufferPointer()),
      (UINT)dof_cs->GetBufferSize());

  dof_pso_.Finalize();
}

void DepthOfField::ResizeBuffers(UINT width, UINT height, UINT mip_level, DXGI_FORMAT format) {
  if (width != width_ || height != height_ ||
      mip_level_ != mip_level || format != format_) {

    width_ = width;
    height_ = height;
    mip_level_ = mip_level;
    format_ = format;

    blur_buffer_.Create(L"FoV Blur Map 00", width_, height_, 1, format);
    color_buffer_.Create(L"FoV Blur Map 01", width_, height_, 1, format);
    depth_buffer_.Create(L"DoF DepthBuffer", width_, height_, Graphics::Core.DepthStencilFormat);
    dof_buffer_.Create(L"DoF Standard Map 00", width_, height_, 1, format);
  }

}

void DepthOfField::Render(ColorBuffer& input) {

  ResizeBuffers(input.Width(), input.Height(), input.MipCount(), input.Format());

  auto& context = GraphicsContext::Begin(L"Blur Compute Context 1");
  context.CopyBuffer(blur_buffer_, input);
  //  context.TransitionResource(Graphics::Core.DepthBuffer(), D3D12_RESOURCE_STATE_DEPTH_WRITE);
  context.Finish(true);

  int blur_count = GameSetting::GetIntValue("DofBlurIterateCount");
  Graphics::Utility::Blur(blur_buffer_, blur_count);

  RenderInternal(input);
}

void DepthOfField::RenderInternal(ColorBuffer& input) {
  auto& context = ComputeContext::Begin(L"DoF Compute Context 2");
  context.CopyBuffer(depth_buffer_, Graphics::Core.DepthBuffer());
  context.TransitionResource(Graphics::Core.DepthBuffer(), D3D12_RESOURCE_STATE_DEPTH_WRITE, true);

  context.SetRootSignature(dof_root_signature_);

  context.CopyBuffer(color_buffer_, input);
  context.TransitionResource(color_buffer_, D3D12_RESOURCE_STATE_GENERIC_READ);
  context.TransitionResource(blur_buffer_, D3D12_RESOURCE_STATE_GENERIC_READ);
  context.TransitionResource(depth_buffer_, D3D12_RESOURCE_STATE_GENERIC_READ);
  context.TransitionResource(dof_buffer_, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, true);

  //  
  D3D12_CPU_DESCRIPTOR_HANDLE handles[] = { blur_buffer_.Srv(), depth_buffer_.DepthSRV(), color_buffer_.Srv() };
  D3D12_CPU_DESCRIPTOR_HANDLE handles2[] = { dof_buffer_.Uav() };

  float focus = 10.0f;
  float znear = MainCamera.ZNear();
  float zfar = MainCamera.ZFar();
  float aspect = MainCamera.Aspect();

  vector<float> camera_data = { focus , MainCamera.ZNear(), 
      MainCamera.ZFar(), aspect };

  context.SetPipelineState(dof_pso_);
  context.SetRoot32BitConstants(0, (int)camera_data.size(), camera_data.data(), 0);
  context.SetDynamicDescriptors(1, 0, _countof(handles), handles);
  context.SetDynamicDescriptors(2, 0, _countof(handles2), handles2);

  context.Dispatch(width_, height_, 1);
  context.CopyBuffer(Graphics::Core.DisplayPlane(), dof_buffer_);
  context.Finish(true);
}

};