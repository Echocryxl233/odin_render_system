#include "depth_of_field.h"
#include "graphics_core.h"
#include "command_context.h"
#include "camera.h"

namespace PostProcess {

using namespace GameCore;

DepthOfField DoF;

void DepthOfField::Initialize() {
  width_ = Graphics::Core.Width();
  height_ = Graphics::Core.Height();
  //Graphics::Core.BackBufferFormat

  auto format = Graphics::Core.BackBufferFormat;
  
  blur_buffer_0_.Create(L"DoF Blur Map 00", width_, height_, 1, format);
  unknown_buffer_.Create(L"DoF Blur Map 01", width_, height_, 1, format);
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

void DepthOfField::OnResize(UINT width, UINT height) {
  if (width != width_ || height != height_) {
    width_ = width;
    height_ = height;

    blur_buffer_0_.Destroy();
    unknown_buffer_.Destroy();
    dof_buffer_.Destroy();

    auto format = Graphics::Core.BackBufferFormat;

    blur_buffer_0_.Create(L"FoV Blur Map 00", width_, height_, 1, format);
    unknown_buffer_.Create(L"FoV Blur Map 01", width_, height_, 1, format);
    depth_buffer_.Create(L"DoF DepthBuffer", width_, height_, Graphics::Core.DepthStencilFormat);
    dof_buffer_.Create(L"DoF Standard Map 00", width_, height_, 1, format);
  }

}

void DepthOfField::Render(ColorBuffer& input, int blur_count) {
  Blur(input, blur_count);
  auto& context = GraphicsContext::Begin(L"Blur Compute Context");
  context.CopyBuffer(depth_buffer_, Graphics::Core.DepthBuffer());
  context.TransitionResource(Graphics::Core.DepthBuffer(), D3D12_RESOURCE_STATE_DEPTH_WRITE);
  context.Finish(true);
  RenderInternal(input);
}

void DepthOfField::Blur(ColorBuffer& input, int blur_count) {
  auto weights = CalculateGaussWeights(2.5f);
  UINT radius = (UINT)weights.size()/2;

  auto& context = ComputeContext::Begin(L"Blur Compute Context");
  

  context.SetRootSignature(blur_root_signature_);

  context.SetRoot32BitConstants(0, 1, &radius, 0);
  context.SetRoot32BitConstants(0, (int)weights.size(), weights.data(), 1);

  context.CopyBuffer(blur_buffer_0_, input);
  //  context.CopyBuffer(blur_buffer_1_, input);

  context.TransitionResource(blur_buffer_0_, D3D12_RESOURCE_STATE_GENERIC_READ);
  context.TransitionResource(unknown_buffer_, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, true);

  for (int i=0; i<blur_count; ++i) {
    D3D12_CPU_DESCRIPTOR_HANDLE handles[] = { blur_buffer_0_.Srv() };
    D3D12_CPU_DESCRIPTOR_HANDLE handles2[] = { unknown_buffer_.Uav() };

    context.SetPipelineState(horizontal_pso_);
    context.SetDynamicDescriptors(1, 0, _countof(handles), handles);
    context.SetDynamicDescriptors(2, 0, _countof(handles2), handles2);

    UINT group_x_count = (UINT)ceilf(width_ /256.0f);
    context.Dispatch(group_x_count, height_, 1);

    context.CopyBuffer(blur_buffer_0_,  unknown_buffer_);

    context.TransitionResource(blur_buffer_0_, D3D12_RESOURCE_STATE_GENERIC_READ);
    context.TransitionResource(unknown_buffer_, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, true);
  }

  for (int i=0; i<blur_count; ++i) {

    D3D12_CPU_DESCRIPTOR_HANDLE handles3[] = { blur_buffer_0_.Srv() };
    D3D12_CPU_DESCRIPTOR_HANDLE handles4[] = { unknown_buffer_.Uav() };

    context.SetPipelineState(vertical_pso_);
    context.SetDynamicDescriptors(1, 0, _countof(handles3), handles3);
    context.SetDynamicDescriptors(2, 0, _countof(handles4), handles4);

    UINT group_y_count = (UINT)ceilf(height_/256.0f);
    context.Dispatch(width_, group_y_count, 1);

    context.CopyBuffer(blur_buffer_0_,  unknown_buffer_);

    context.TransitionResource(blur_buffer_0_, D3D12_RESOURCE_STATE_GENERIC_READ);
    context.TransitionResource(unknown_buffer_, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, true);
  }

  context.Finish(true);
}

void DepthOfField::RenderInternal(ColorBuffer& input) {
  auto& context = ComputeContext::Begin(L"DoF Compute Context");
  context.SetRootSignature(dof_root_signature_);

  context.CopyBuffer(unknown_buffer_, input);
  context.TransitionResource(unknown_buffer_, D3D12_RESOURCE_STATE_GENERIC_READ);
  context.TransitionResource(blur_buffer_0_, D3D12_RESOURCE_STATE_GENERIC_READ);
  context.TransitionResource(depth_buffer_, D3D12_RESOURCE_STATE_GENERIC_READ);
  context.TransitionResource(dof_buffer_, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, true);

  D3D12_CPU_DESCRIPTOR_HANDLE handles[] = { blur_buffer_0_.Srv(), depth_buffer_.DepthSRV(), unknown_buffer_.Srv() };
  D3D12_CPU_DESCRIPTOR_HANDLE handles2[] = { dof_buffer_.Uav() };

  float focus = 5.0f;
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

  context.Finish(true);
}

vector<float> DepthOfField::CalculateGaussWeights(float sigma) {
  float two_sigma_sigma = 2.0f * sigma* sigma;

  int blur_radius = (int)ceil(2.0f * sigma);

  vector<float> weights(blur_radius*2 + 1);
  float weight_sum = 0.0f;
    
  for (int i=-blur_radius; i<=blur_radius; ++i) {
    float x = (float)i;
    float weight = expf(-x*x / two_sigma_sigma);
    //  weights.emplace_back(weight);

    weights[i+blur_radius] = weight;

    weight_sum += weight;
  }

  for (auto& weight : weights) {
    weight/=weight_sum;
  }

  return weights;
}

};