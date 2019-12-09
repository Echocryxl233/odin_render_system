#pragma once

#include "utility.h"

#ifndef GPURESOURCE_H
#define GPURESOURCE_H


class GpuResource
{
friend class CommandContext;

 public:
  GpuResource();
  virtual ~GpuResource();

  ID3D12Resource* GetResource() { return resource_.Get(); }
  D3D12_GPU_VIRTUAL_ADDRESS GetVirtualAddress() { return resource_->GetGPUVirtualAddress(); }
  void Destroy() ;

  D3D12_GPU_VIRTUAL_ADDRESS GetGpuVirtualAddress() const { return gpu_virtual_address_; }

 protected:
  Microsoft::WRL::ComPtr<ID3D12Resource> resource_;
  //  D3D12_RESOURCE_STATES usage_usage_;
  D3D12_RESOURCE_STATES current_state_;
  D3D12_GPU_VIRTUAL_ADDRESS gpu_virtual_address_;
  wstring name_;
};

inline void GpuResource::Destroy() {
  //  assert(resource_ != nullptr);
  resource_.Reset();
}

#endif // !GPURESOURCE_H



