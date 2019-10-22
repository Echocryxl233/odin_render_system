#pragma once

#include "utility.h"

class RootParameter {
 public:
  RootParameter() {
    root_parameter_.ParameterType = (D3D12_ROOT_PARAMETER_TYPE)0xFFFFFFFF;
  }
  void InitAsConstants(UINT num32BitValues, UINT shaderRegister, UINT registerSpace = 0, 
      D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL) {
    //  root_parameter_.InitAsConstants(num32BitValues, shaderRegister, registerSpace, visibility);
    root_parameter_.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    root_parameter_.ShaderVisibility = visibility;
    root_parameter_.Constants.Num32BitValues = num32BitValues;
    root_parameter_.Constants.ShaderRegister = shaderRegister;
    root_parameter_.Constants.RegisterSpace = registerSpace;
  }

  void InitAsConstantBufferView( UINT shaderRegister, 
      UINT registerSpace = 0, 
      D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL) {
    root_parameter_.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    root_parameter_.ShaderVisibility = visibility;
    root_parameter_.Descriptor.ShaderRegister = shaderRegister;
    root_parameter_.Descriptor.RegisterSpace = registerSpace;
    //  root_parameter_.InitAsConstantBufferView(shaderRegister, registerSpace, visibility);
  }

  void InitAsShaderResourceView(UINT shaderRegister, 
      UINT registerSpace = 0, 
      D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL) {
    root_parameter_.ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
    root_parameter_.ShaderVisibility = visibility;
    root_parameter_.Descriptor.ShaderRegister = shaderRegister;
    root_parameter_.Descriptor.RegisterSpace = registerSpace;
    //  root_parameter_.InitAsShaderResourceView(shaderRegister, registerSpace, visibility);
  }

  //void InitAsDescriptorTable(UINT numDescriptorRanges, 
  //  const D3D12_DESCRIPTOR_RANGE* pDescriptorRanges, 
  //  D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL) {

  //  root_parameter_.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
  //  root_parameter_.ShaderVisibility = visibility;

  //  root_parameter_.DescriptorTable.NumDescriptorRanges = numDescriptorRanges;
  //  root_parameter_.DescriptorTable.pDescriptorRanges = pDescriptorRanges;

  //  //  root_parameter_.InitAsDescriptorTable(numDescriptorRanges, pDescriptorRanges, visibility);
  //}
  
  void InitAsDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE range_type, UINT descriptor_count, 
      UINT baseShaderRegister, UINT registerSpace = 0, D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL) {
    InitAsDescriptorTable(1, visibility);
    SetDescriptorRange(0, range_type, descriptor_count, baseShaderRegister, registerSpace);
  }

  void InitAsDescriptorTable(UINT range_count, D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL) {
    root_parameter_.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    root_parameter_.ShaderVisibility = visibility;

    root_parameter_.DescriptorTable.NumDescriptorRanges = range_count;
    root_parameter_.DescriptorTable.pDescriptorRanges = new D3D12_DESCRIPTOR_RANGE[range_count];
  }

  void SetDescriptorRange(UINT range_index, D3D12_DESCRIPTOR_RANGE_TYPE range_type, UINT descriptor_count, 
      UINT baseShaderRegister, UINT registerSpace = 0) {
    D3D12_DESCRIPTOR_RANGE* range = const_cast<D3D12_DESCRIPTOR_RANGE*>(root_parameter_.DescriptorTable.pDescriptorRanges + range_index);
    range->RangeType = range_type;
    range->NumDescriptors = descriptor_count;
    range->BaseShaderRegister = baseShaderRegister;
    range->RegisterSpace = registerSpace;
    range->OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
  }

  const D3D12_ROOT_PARAMETER& operator() ( void ) const { return root_parameter_; }

 private:
   // CD3DX12_ROOT_PARAMETER root_parameter_;
   D3D12_ROOT_PARAMETER root_parameter_;
};


class RootSignature
{
friend class DynamicDescriptorHeap;

 public:
  ID3D12RootSignature* GetSignature() const { return signature_; }

  RootParameter& operator[](size_t index) const {
    assert((index>=0 && index< parameter_count_) && "Get RootParameter index error");
    return (parameters_.get())[index];
  }

  void Reset(UINT root_param_count, UINT sampler_count) {
    if (0 < root_param_count) {
      parameters_.reset(new RootParameter[root_param_count]);
    }
    else {
      parameters_ = nullptr;
    }
    parameter_count_ = root_param_count;


    if (0 < sampler_count) {
      samplers_.reset(new D3D12_STATIC_SAMPLER_DESC[sampler_count]);
    }
    else {
      samplers_ = nullptr;
    }
    sampler_count_ = sampler_count;
    sampler_index_ = 0;
  }

  void InitSampler(UINT shader_register, const D3D12_SAMPLER_DESC& desc,
      D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL);


  void Finalize();

 private:
  ID3D12RootSignature* signature_;
  uint32_t descriptor_table_bit_map_;
  uint32_t sampler_table_bit_map_;
  UINT parameter_count_;
  UINT sampler_count_;
  UINT sampler_index_;
  uint32_t descriptor_sizes[16];
  std::unique_ptr<RootParameter[]> parameters_;
  std::unique_ptr<D3D12_STATIC_SAMPLER_DESC[]> samplers_;
};

