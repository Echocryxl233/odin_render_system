#include "root_signature.h"
#include "graphics_core.h"

using Microsoft::WRL::ComPtr;

void RootSignature::InitSampler(UINT shader_register, const D3D12_SAMPLER_DESC& desc,
    D3D12_SHADER_VISIBILITY visibility) {

  assert(sampler_index_ < sampler_count_ && "The index of sampler is over sampler count");

  D3D12_STATIC_SAMPLER_DESC& static_sampler_desc = samplers_[sampler_index_++];
  static_sampler_desc.Filter = desc.Filter;
  static_sampler_desc.AddressU = desc.AddressU;
  static_sampler_desc.AddressV = desc.AddressV;
  static_sampler_desc.AddressW = desc.AddressW;
  static_sampler_desc.MipLODBias = desc.MipLODBias;
  static_sampler_desc.MaxAnisotropy = desc.MaxAnisotropy;
  static_sampler_desc.ComparisonFunc = desc.ComparisonFunc;
  static_sampler_desc.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
  static_sampler_desc.MinLOD = desc.MinLOD;
  static_sampler_desc.MaxLOD = desc.MaxLOD;
  static_sampler_desc.ShaderRegister = shader_register;
  static_sampler_desc.RegisterSpace = 0;
  static_sampler_desc.ShaderVisibility = visibility;

  //if (StaticSamplerDesc.AddressU == D3D12_TEXTURE_ADDRESS_MODE_BORDER ||
  //  StaticSamplerDesc.AddressV == D3D12_TEXTURE_ADDRESS_MODE_BORDER ||
  //  StaticSamplerDesc.AddressW == D3D12_TEXTURE_ADDRESS_MODE_BORDER)
  //{
  //  WARN_ONCE_IF_NOT(
  //    // Transparent Black
  //    NonStaticSamplerDesc.BorderColor[0] == 0.0f &&
  //    NonStaticSamplerDesc.BorderColor[1] == 0.0f &&
  //    NonStaticSamplerDesc.BorderColor[2] == 0.0f &&
  //    NonStaticSamplerDesc.BorderColor[3] == 0.0f ||
  //    // Opaque Black
  //    NonStaticSamplerDesc.BorderColor[0] == 0.0f &&
  //    NonStaticSamplerDesc.BorderColor[1] == 0.0f &&
  //    NonStaticSamplerDesc.BorderColor[2] == 0.0f &&
  //    NonStaticSamplerDesc.BorderColor[3] == 1.0f ||
  //    // Opaque White
  //    NonStaticSamplerDesc.BorderColor[0] == 1.0f &&
  //    NonStaticSamplerDesc.BorderColor[1] == 1.0f &&
  //    NonStaticSamplerDesc.BorderColor[2] == 1.0f &&
  //    NonStaticSamplerDesc.BorderColor[3] == 1.0f,
  //    "Sampler border color does not match static sampler limitations");

  //  if (NonStaticSamplerDesc.BorderColor[3] == 1.0f)
  //  {
  //    if (NonStaticSamplerDesc.BorderColor[0] == 1.0f)
  //      StaticSamplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
  //    else
  //      StaticSamplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
  //  }
  //  else
  //    StaticSamplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
  //}
}

void RootSignature::Finalize() {
  CD3DX12_ROOT_SIGNATURE_DESC root_signature_desc;
  root_signature_desc.NumParameters = parameter_count_;
  root_signature_desc.NumStaticSamplers = sampler_count_;
  root_signature_desc.pParameters = (const D3D12_ROOT_PARAMETER*)parameters_.get();
  root_signature_desc.pStaticSamplers = (const D3D12_STATIC_SAMPLER_DESC*)samplers_.get();
  root_signature_desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

  for (UINT param_index = 0; param_index < parameter_count_; ++param_index) {
    const D3D12_ROOT_PARAMETER& root_parameter = root_signature_desc.pParameters[param_index];
    descriptor_sizes[param_index] = 0;

    if (root_parameter.ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE) {
      if (root_parameter.DescriptorTable.pDescriptorRanges->RangeType == D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER) {
        sampler_table_bit_map_ |= (1 << param_index);
      }
      else {
        descriptor_table_bit_map_ |= (1 << param_index);
      }

      for (UINT table_range = 0; table_range< root_parameter.DescriptorTable.NumDescriptorRanges; ++table_range) {
        descriptor_sizes[param_index] += root_parameter.DescriptorTable.pDescriptorRanges[table_range].NumDescriptors;
      }
    }
  }

  ComPtr<ID3DBlob> output_blob = nullptr;
  ComPtr<ID3DBlob> error_blob = nullptr;
  HRESULT hr = D3D12SerializeRootSignature(&root_signature_desc, 
    D3D_ROOT_SIGNATURE_VERSION_1,
    output_blob.GetAddressOf(), 
    error_blob.GetAddressOf());

  if (error_blob != nullptr)
  {
    ::OutputDebugStringA((char*)error_blob->GetBufferPointer());
  }
  ThrowIfFailed(hr);

  ThrowIfFailed(Graphics::Core.Device()->CreateRootSignature(
    0,
    output_blob->GetBufferPointer(),
    output_blob->GetBufferSize(),
    IID_PPV_ARGS(&signature_)));
}
