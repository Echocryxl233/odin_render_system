#include "graphics/pipeline_state.h"
#include "graphics/graphics_core.h"

GraphicsPso::GraphicsPso() {
  ZeroMemory(&pso_desc_, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
  pso_desc_.NodeMask = 1;
  pso_desc_.SampleMask = 0xFFFFFFFFu;
  pso_desc_.SampleDesc.Count = 1;
  pso_desc_.InputLayout.NumElements = 0;
}

void GraphicsPso::CopyDesc(const GraphicsPso& rhs) {
  pso_desc_ = rhs.pso_desc_;
}

void GraphicsPso::SetInputLayout(const D3D12_INPUT_ELEMENT_DESC* descs, UINT element_count) {
  pso_desc_.InputLayout.NumElements = element_count;

  if (element_count > 0) {
    D3D12_INPUT_ELEMENT_DESC* NewElements = (D3D12_INPUT_ELEMENT_DESC*)malloc(sizeof(D3D12_INPUT_ELEMENT_DESC) * element_count);
    memcpy(NewElements, descs, element_count * sizeof(D3D12_INPUT_ELEMENT_DESC));
    input_layouts_.reset((const D3D12_INPUT_ELEMENT_DESC*)NewElements);
  }
  else {
    pso_desc_.InputLayout.pInputElementDescs = nullptr;
  }
}

void GraphicsPso::SetRenderTargetFormats(UINT NumRTVs, const DXGI_FORMAT* RTVFormats, DXGI_FORMAT DSVFormat, UINT MsaaCount, UINT MsaaQuality)
{
  assert((NumRTVs == 0 || RTVFormats != nullptr) && "Null format array conflicts with non-zero length");
  for (UINT i = 0; i < NumRTVs; ++i)
    pso_desc_.RTVFormats[i] = RTVFormats[i];
  for (UINT i = NumRTVs; i < pso_desc_.NumRenderTargets; ++i)
    pso_desc_.RTVFormats[i] = DXGI_FORMAT_UNKNOWN;

  pso_desc_.NumRenderTargets = NumRTVs;
  pso_desc_.DSVFormat = DSVFormat;
  pso_desc_.SampleDesc.Count = MsaaCount;
  pso_desc_.SampleDesc.Quality = MsaaQuality;
}

void GraphicsPso::Finalize() {
  //  pso_desc_.pRootSignature = const_cast<ID3D12RootSignature*>(root_signature_);
  assert(root_signature_ != nullptr && "Finalize graphics pso exception : root signature is nullptr");
  pso_desc_.pRootSignature = root_signature_->GetSignature();
  pso_desc_.InputLayout.pInputElementDescs = input_layouts_.get();
  ASSERT_SUCCESSED(Graphics::Core.Device()->CreateGraphicsPipelineState(&pso_desc_, IID_PPV_ARGS(&pso_)));
}

ComputePso::ComputePso() {
  ZeroMemory(&pso_desc_, sizeof(D3D12_COMPUTE_PIPELINE_STATE_DESC));
  pso_desc_.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
}

void ComputePso::Finalize() {
  //  assert(root_signature_ != nullptr && "ComputePso root signature can't be nullptr");
  
  pso_desc_.pRootSignature = root_signature_->GetSignature();
  //  pso_desc_.pRootSignature = root_sig_;
  ASSERT_SUCCESSED(Graphics::Core.Device()->CreateComputePipelineState(&pso_desc_, IID_PPV_ARGS(&pso_)));
}




