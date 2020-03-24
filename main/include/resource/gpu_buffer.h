#pragma once

#ifndef GPUBUFFER_H
#define GPUBUFFER_H

#include "resource/gpu_resource.h"

class GpuBuffer : public GpuResource
{
 public:
  
  virtual ~GpuBuffer();

  void Create(wstring name, uint32_t element_count, uint32_t element_size, const void* init_data = nullptr);
  //  void Create(wstring name, ID3D12Device* device, uint32_t element_count, uint32_t element_size, const void* init_data = nullptr);

  D3D12_CPU_DESCRIPTOR_HANDLE& Srv() { return srv_; }
  D3D12_CPU_DESCRIPTOR_HANDLE& Uav() { return uav_; }

  uint32_t ElementCount() const { return element_count_; }
  uint32_t ElementSize() const { return element_size_; }
  size_t BufferSize() const { return buffer_size_; }

  D3D12_VERTEX_BUFFER_VIEW VertexBufferView(size_t offset, uint32_t size, uint32_t stride) const;
  D3D12_VERTEX_BUFFER_VIEW VertexBufferView(size_t BaseVertexIndex = 0) const
  {
    size_t offset = BaseVertexIndex * element_size_;
    return VertexBufferView(offset, (uint32_t)(buffer_size_ - offset), element_size_);
  }
  //  D3D12_VERTEX_BUFFER_VIEW VertexBufferView() const;

  
  //  D3D12_INDEX_BUFFER_VIEW IndexBufferView(bool b32_bit = false) const;
  D3D12_INDEX_BUFFER_VIEW IndexBufferView(size_t offset, uint32_t size, bool b32_bit = false) const;
  D3D12_INDEX_BUFFER_VIEW IndexBufferView(size_t StartIndex = 0) const
  {
    size_t offset = StartIndex * element_size_;
    return IndexBufferView(offset, (uint32_t)(buffer_size_ - offset), element_size_ == 4);
  }
 protected:
  GpuBuffer();

  D3D12_RESOURCE_DESC DescribeBuffer();

  virtual void CreateDerivedViews() = 0 ;
  
  uint32_t element_count_;
  uint32_t element_size_;
  size_t buffer_size_;

  D3D12_CPU_DESCRIPTOR_HANDLE srv_;
  D3D12_CPU_DESCRIPTOR_HANDLE uav_;
  D3D12_RESOURCE_FLAGS resource_flags_;
};

//inline D3D12_VERTEX_BUFFER_VIEW GpuBuffer::VertexBufferView()const
//{
//  D3D12_VERTEX_BUFFER_VIEW vbv;
//  vbv.BufferLocation = resource_->GetGPUVirtualAddress();
//  vbv.StrideInBytes = element_size_;
//  vbv.SizeInBytes = buffer_size_;
//
//  return vbv;
//}

inline D3D12_VERTEX_BUFFER_VIEW GpuBuffer::VertexBufferView(size_t Offset, uint32_t Size, uint32_t Stride) const
{
  D3D12_VERTEX_BUFFER_VIEW vbv;
  vbv.BufferLocation = gpu_virtual_address_ + Offset;
  vbv.SizeInBytes = Size;
  vbv.StrideInBytes = Stride;
  return vbv;
}

inline D3D12_INDEX_BUFFER_VIEW GpuBuffer::IndexBufferView(size_t Offset, uint32_t Size, bool b32Bit) const
{
  D3D12_INDEX_BUFFER_VIEW ibv;
  ibv.BufferLocation = gpu_virtual_address_ + Offset;
  ibv.Format = b32Bit ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT;
  ibv.SizeInBytes = Size;
  return ibv;
}

//inline D3D12_INDEX_BUFFER_VIEW GpuBuffer::IndexBufferView(bool b32_bit)const
//{
//  D3D12_INDEX_BUFFER_VIEW ibv;
//  ibv.BufferLocation = resource_->GetGPUVirtualAddress();
//  ibv.Format = b32_bit ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT;;
//  ibv.SizeInBytes = buffer_size_;
//
//  return ibv;
//}

class ByteAddressBuffer : public GpuBuffer {
  
  void CreateDerivedViews() override;
};

class StructuredBuffer : public GpuBuffer {
 public:
  ~StructuredBuffer() {
    counter_buffer_.Destroy();
    GpuBuffer::Destroy();
  }
  void CreateDerivedViews() override;

  ByteAddressBuffer& GetCounterBuffer(void) { return counter_buffer_; }

  const D3D12_CPU_DESCRIPTOR_HANDLE& GetCounterSRV(CommandContext& Context);
  const D3D12_CPU_DESCRIPTOR_HANDLE& GetCounterUAV(CommandContext& Context);

 private:
  ByteAddressBuffer counter_buffer_;
};



#endif // !GPUBUFFER_H




