#pragma once

#ifndef DESCRIPTORALLOCATOR_H
#define DESCRIPTORALLOCATOR_H

#include "utility.h"

using namespace std;

//  const UINT kDescriptorCount = 256;
class DescriptorAllocatorManager;

class DescriptorHandle {

 public:
  DescriptorHandle() {
    cpu_handle_.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
    gpu_handle_.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
  };

  DescriptorHandle(D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle) 
      : cpu_handle_(cpu_handle)
      {
    gpu_handle_.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
  };

  DescriptorHandle(D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle,
      D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle) 
      : cpu_handle_(cpu_handle),
        gpu_handle_(gpu_handle)
  {
  };

  D3D12_CPU_DESCRIPTOR_HANDLE& CpuHandle() { return cpu_handle_; }
  D3D12_GPU_DESCRIPTOR_HANDLE& GpuHandle() { return gpu_handle_; }

  void operator+=(UINT offset_size) {
    if (cpu_handle_.ptr != D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN) {
      cpu_handle_.ptr += offset_size;
    }
    
    if (gpu_handle_.ptr != D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN) {
      gpu_handle_.ptr += offset_size;
    }
  }

  DescriptorHandle operator+ (UINT offset_size) {
    DescriptorHandle ret = *this;
    ret += offset_size;
    return ret;
  }

 private:
  D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle_;
  D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle_;
};

//class DescriptorPool {
//public:
//  //  D3D12_CPU_DESCRIPTOR_HANDLE Allocate(ID3D12Device* device, uint32_t descriptor_count) ;
//  ID3D12DescriptorHeap* RequestNewHeap(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type);
//private:
//  vector<Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>> allocaltor_pool_;
//};



class DescriptorAllocatorManager {

  class DescriptorAllocator
  {
  public:
    static const UINT kDescriptorCount = 256;
    DescriptorAllocator(D3D12_DESCRIPTOR_HEAP_TYPE type);
    virtual ~DescriptorAllocator();

    D3D12_CPU_DESCRIPTOR_HANDLE Allocate(ID3D12Device* device, uint32_t descriptor_count) ;

  protected:
    ID3D12DescriptorHeap* RequestNewHeap(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type);
    vector<Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>> allocaltor_pool_;

    UINT remainning_descriptor_count_ = kDescriptorCount;
    D3D12_CPU_DESCRIPTOR_HANDLE current_handle_;
    ID3D12DescriptorHeap* current_heap_;
    D3D12_DESCRIPTOR_HEAP_TYPE type_;
  };

 public:
  DescriptorAllocatorManager();
//  ~DescriptorAllocatorManager();
//
  static DescriptorAllocatorManager& Instance() {
    static DescriptorAllocatorManager instance;
    return instance;
  }

  //  ID3D12Device* device;

  D3D12_CPU_DESCRIPTOR_HANDLE Allocate(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t descriptor_count = 1) ;

 private:
  //  std::map<D3D12_DESCRIPTOR_HEAP_TYPE, unique_ptr<DescriptorAllocator>> allocators_;
  std::shared_ptr<DescriptorAllocator> allocators_[(int)D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];
};



#endif // !DESCRIPTORALLOCATOR_H




