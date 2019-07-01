#include "FrameResource.h"

namespace OdinRenderSystem {

FrameResource::FrameResource(ID3D12Device* device, UINT pass_CB_count, UINT object_CB_count, 
  UINT material_count, UINT wave_vertex_count)
{
  ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT,
    IID_PPV_ARGS(CmdListAlloc.GetAddressOf())));

  PassCB = std::make_unique<UploadBuffer<PassConstants>>(device, pass_CB_count, true);
  ObjectCB = std::make_unique<UploadBuffer<ObjectConstants>>(device, object_CB_count, true);

  MaterialCB = std::make_unique<UploadBuffer<MaterialConstants>>(device, material_count, true);

  //  WavesVB = std::make_unique<UploadBuffer<Vertex>>(device, wave_vertex_count, false);
}


FrameResource::~FrameResource()
{
}

}
