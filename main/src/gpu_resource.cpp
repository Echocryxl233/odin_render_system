#include "gpu_resource.h"



GpuResource::GpuResource()
    : gpu_virtual_address_(D3D12_GPU_VIRTUAL_ADDRESS_NULL),
      resource_(nullptr)
{
}


GpuResource::~GpuResource()
{
}
