#include "postprocess/bloom.h"
#include "graphics_core.h"
#include "graphics_utility.h"

using namespace PostProcess;

namespace PostProcess {
  Bloom BloomEffect;
};

void Bloom::Initialize() {
  width_ = (UINT)Graphics::Core.Width();
  height_ = (UINT)Graphics::Core.Height();

  auto format = Graphics::Core.BackBufferFormat;

  bloom_buffer_.Create(L"bloom buffer Map 00", width_, height_, 1, format);
  temp_buffer_.Create(L"bloom temp Map 01", width_, height_, 1, format);

  auto bloom_cs = d3dUtil::CompileShader(L"shaders/bloom.hlsl", nullptr, "Bloom", "cs_5_0");

  root_signature_.Reset(2, 0);
  //  root_signature_[0].InitAsConstants(4, 0);
  root_signature_[0].InitAsDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 0);  //  0, D3D12_SHADER_VISIBILITY_PIXEL
  root_signature_[1].InitAsDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);
  root_signature_.Finalize();

  pso_.SetComputeShader(bloom_cs);
  pso_.SetRootSignature(root_signature_);
  pso_.Finalize();


  auto composite_cs = d3dUtil::CompileShader(L"shaders/bloom_composite.hlsl", nullptr, "BloomComposite", "cs_5_0");

  //root_signature_.Reset(2, 0);
  ////  root_signature_[0].InitAsConstants(4, 0);
  //root_signature_[0].InitAsDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);  //  0, D3D12_SHADER_VISIBILITY_PIXEL
  //root_signature_[1].InitAsDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);
  //root_signature_.Finalize();

  composite_pso_.SetComputeShader(composite_cs);
  composite_pso_.SetRootSignature(root_signature_);
  composite_pso_.Finalize();
};

void Bloom::ResizeBuffers(UINT width, UINT height, UINT mip_level, DXGI_FORMAT format) {
  if (width != width_ || height != height_ ||
    mip_level_ != mip_level || format != format_) {

    width_ = width;
    height_ = height;
    mip_level_ = mip_level;
    format_ = format;

    bloom_buffer_.Create(L"bloom buffer", width_, height_, mip_level_, format_);

  }
}

void Bloom::Render(ColorBuffer& input) {

  ResizeBuffers(input.Width(), input.Height(), input.MipCount(), input.Format());

  D3D12_CPU_DESCRIPTOR_HANDLE handles[] = { input.Srv(), temp_buffer_.Srv() };
  D3D12_CPU_DESCRIPTOR_HANDLE handles2[] = { bloom_buffer_.Uav() };

  auto& context = ComputeContext::Begin(L"Bloom Compute Context 2");
  //context.CopyBuffer(bloom_buffer_, input);
  context.CopyBuffer(temp_buffer_, input);

  context.TransitionResource(temp_buffer_, D3D12_RESOURCE_STATE_GENERIC_READ);
  context.TransitionResource(bloom_buffer_, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, true);

  context.SetRootSignature(root_signature_);
  context.SetPipelineState(pso_);



  context.SetDynamicDescriptors(0, 0, _countof(handles), handles);
  context.SetDynamicDescriptors(1, 0, _countof(handles2), handles2);

  UINT group_x_count = (UINT)ceilf(width_ / 256.0f);
  context.Dispatch(width_, height_, 1);

  context.Finish(true);

  Graphics::Utility::Blur(bloom_buffer_, 15);

  auto& context2 = ComputeContext::Begin(L"bloom composite Compute Context 2");
  context2.CopyBuffer(temp_buffer_, bloom_buffer_);

  context2.TransitionResource(temp_buffer_, D3D12_RESOURCE_STATE_GENERIC_READ);
  context2.TransitionResource(bloom_buffer_, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, true);

  context2.SetRootSignature(root_signature_);
  context2.SetPipelineState(composite_pso_);


  context2.SetDynamicDescriptors(0, 0, _countof(handles), handles);
  context2.SetDynamicDescriptors(1, 0, _countof(handles2), handles2);

  context2.Dispatch(width_, height_, 1);

  context2.Finish();
}