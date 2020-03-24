#include "graphics/sampler_manager.h"
#include "graphics/descriptor_allocator.h"
#include "graphics/graphics_core.h"

D3D12_CPU_DESCRIPTOR_HANDLE SamplerDesc::CreateDescriptor() {
  auto handle = DescriptorAllocatorManager::Instance().Allocate(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
  Graphics::Core.Device()->CreateSampler(this, handle);
  return handle;
}