#include "resource/gpu_buffer.h"
#include "graphics/graphics_core.h"
#include "graphics/command_context.h"

using namespace Graphics;

GpuBuffer::GpuBuffer()
    : element_count_(0), element_size_(0), buffer_size_(0)
{
  resource_flags_ = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
  uav_.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
  srv_.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
}


GpuBuffer::~GpuBuffer()
{
  Destroy();
}

void GpuBuffer::Create(wstring name, uint32_t element_count, uint32_t element_size, const void* init_data) {
  Destroy();

  element_count_ = element_count;
  element_size_ = element_size;
  buffer_size_ = element_count_ * element_size_;

  D3D12_HEAP_PROPERTIES properties = {};
  properties.Type = D3D12_HEAP_TYPE_DEFAULT;
  properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
  properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
  properties.CreationNodeMask = 1;
  properties.VisibleNodeMask = 1;

  current_state_ = D3D12_RESOURCE_STATE_COMMON;

  auto desc = DescribeBuffer();

  ASSERT_SUCCESSED(Core.Device()->CreateCommittedResource(&properties, D3D12_HEAP_FLAG_NONE, &desc, 
    current_state_, nullptr, IID_PPV_ARGS(resource_.GetAddressOf())));

  gpu_virtual_address_ = resource_->GetGPUVirtualAddress();
  resource_->SetName(name.c_str());

  if (init_data) {
    CommandContext::InitializeBuffer(*this, init_data, buffer_size_);
  }
  
  CreateDerivedViews();
}

//void GpuBuffer::Create(wstring name, ID3D12Device* device, uint32_t element_count, 
//    uint32_t element_size, const void* init_data) {
//
//  Destroy();
//
//  element_count_ = element_count;
//  element_size_ = element_size;
//  buffer_size_ = element_count_ * element_size_;
//
//  D3D12_HEAP_PROPERTIES properties = {};
//  properties.Type = D3D12_HEAP_TYPE_DEFAULT;
//  properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
//  properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
//  properties.CreationNodeMask = 1;
//  properties.VisibleNodeMask = 1;
//
//  current_state_ = D3D12_RESOURCE_STATE_COMMON;
//
//  auto desc = DescribeBuffer();
//
//  ASSERT_SUCCESSED(device->CreateCommittedResource(&properties, D3D12_HEAP_FLAG_NONE, &desc,
//    current_state_, nullptr, IID_PPV_ARGS(resource_.GetAddressOf())));
//
//  gpu_virtual_address_ = resource_->GetGPUVirtualAddress();
//  resource_->SetName(name.c_str());
//
//  if (init_data) {
//    CommandContext::InitializeBuffer(*this, init_data, buffer_size_);
//  }
//
//  CreateDerivedViews();
//}

D3D12_RESOURCE_DESC GpuBuffer::DescribeBuffer() {

  D3D12_RESOURCE_DESC resource_desc = {};
  resource_desc.Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_BUFFER;
  resource_desc.Alignment = 0;
  resource_desc.Width = buffer_size_;
  resource_desc.Height = 1;
  resource_desc.DepthOrArraySize = 1;
  resource_desc.MipLevels = 1;
  resource_desc.Format = DXGI_FORMAT_UNKNOWN;
  resource_desc.SampleDesc.Quality = 0;
  resource_desc.SampleDesc.Count = 1;
  resource_desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
  resource_desc.Flags = resource_flags_;

  return resource_desc;
}

void ByteAddressBuffer::CreateDerivedViews()
{
  D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
  SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
  SRVDesc.Format = DXGI_FORMAT_R32_TYPELESS;
  SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  SRVDesc.Buffer.NumElements = (UINT)buffer_size_ / 4;
  SRVDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;

  if (srv_.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN) {
    srv_ = DescriptorAllocatorManager::Instance().Allocate(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
  }
    
    
  Graphics::Core.Device()->CreateShaderResourceView(resource_.Get(), &SRVDesc, srv_);

  D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc = {};
  UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
  UAVDesc.Format = DXGI_FORMAT_R32_TYPELESS;
  UAVDesc.Buffer.NumElements = (UINT)buffer_size_ / 4;
  UAVDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;

  if (uav_.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN) {
    uav_ = DescriptorAllocatorManager::Instance().Allocate(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
  }
    
  Graphics::Core.Device()->CreateUnorderedAccessView(resource_.Get(), nullptr, &UAVDesc, uav_);
}

void StructuredBuffer::CreateDerivedViews() {
  D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
  srv_desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
  srv_desc.Format = DXGI_FORMAT_UNKNOWN;
  srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  srv_desc.Buffer.NumElements = element_count_;
  srv_desc.Buffer.StructureByteStride = element_size_;
  srv_desc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
 
  if (srv_.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN) {
    srv_ = DescriptorAllocatorManager::Instance().Allocate(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
  }
  Graphics::Core.Device()->CreateShaderResourceView(resource_.Get(), &srv_desc, srv_);

  D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
  uav_desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
  uav_desc.Format = DXGI_FORMAT_UNKNOWN;
  uav_desc.Buffer.CounterOffsetInBytes = 0;
  uav_desc.Buffer.NumElements = element_count_;
  uav_desc.Buffer.StructureByteStride = element_size_;
  uav_desc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

  counter_buffer_.Create(L"StructuredBuffer::Counter", 1, 4);

  if (uav_.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN) {
    uav_ = DescriptorAllocatorManager::Instance().Allocate(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
  }
  Graphics::Core.Device()->CreateUnorderedAccessView(resource_.Get(), counter_buffer_.GetResource(), &uav_desc, uav_);
}

const D3D12_CPU_DESCRIPTOR_HANDLE& StructuredBuffer::GetCounterSRV(CommandContext& Context)
{
  Context.TransitionResource(counter_buffer_, D3D12_RESOURCE_STATE_GENERIC_READ);
  return counter_buffer_.Srv();
}

const D3D12_CPU_DESCRIPTOR_HANDLE& StructuredBuffer::GetCounterUAV(CommandContext& Context)
{
  Context.TransitionResource(counter_buffer_, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
  return counter_buffer_.Uav();
}
