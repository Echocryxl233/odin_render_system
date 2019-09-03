#pragma once

#include "utility.h"

class RootParameter {
 public:
  RootParameter() {
    root_parameter_.ParameterType = (D3D12_ROOT_PARAMETER_TYPE)0xFFFFFFFF;
  }
  void InitAsConstantBufferView( UINT shaderRegister, 
    UINT registerSpace = 0, 
    D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL) {
    
    root_parameter_.InitAsConstantBufferView(shaderRegister, registerSpace, visibility);
  }

  void InitAsShaderResourceView(UINT shaderRegister, 
    UINT registerSpace = 0, 
    D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL) {

    root_parameter_.InitAsShaderResourceView(shaderRegister, registerSpace, visibility);
  }

  void InitAsDescriptorTable(UINT numDescriptorRanges, 
    const D3D12_DESCRIPTOR_RANGE* pDescriptorRanges, 
    D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL) {

    root_parameter_.InitAsDescriptorTable(numDescriptorRanges, pDescriptorRanges, visibility);
  }
    

  void InitAsDescriptorTable(UINT numDescriptorRanges, D3D12_DESCRIPTOR_RANGE_TYPE range_type, UINT numDescriptors, 
      UINT baseShaderRegister, UINT registerSpace = 0) {
    
    CD3DX12_DESCRIPTOR_RANGE cbv_table;
    cbv_table.Init(range_type, numDescriptors, baseShaderRegister, registerSpace);
    root_parameter_.InitAsDescriptorTable(numDescriptorRanges, &cbv_table, D3D12_SHADER_VISIBILITY_ALL);
   }



 private:
   CD3DX12_ROOT_PARAMETER root_parameter_;
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

