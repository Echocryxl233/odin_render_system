#include "sampler_manager.h"
#include "descriptor_allocator.h"
#include "graphics_core.h"

D3D12_CPU_DESCRIPTOR_HANDLE SamplerDesc::CreateDescriptor() {
  auto handle = DescriptorAllocatorManager::Instance().Allocate(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
  Graphics::Core.Device()->CreateSampler(this, handle);
  return handle;
}