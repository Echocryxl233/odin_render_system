#include "FrameResource.h"

namespace OdinRenderSystem {

FrameResource::FrameResource(ID3D12Device* device, UINT pass_CB_count, UINT object_CB_count, UINT max_instance_count,
  UINT material_count, UINT wave_vertex_count)
{
  ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT,
    IID_PPV_ARGS(CmdListAlloc.GetAddressOf())));

  PassCB = std::make_unique<UploadBuffer<PassConstants>>(device, pass_CB_count, true);
  ObjectCB = std::make_unique<UploadBuffer<ObjectConstants>>(device, object_CB_count, true);
  SsaoCB = std::make_unique<UploadBuffer<SsaoConstants>>(device, 1, true);

  MaterialBuffer = std::make_unique<UploadBuffer<MaterialData>>(device, material_count, false);

  for (int i = (int)RenderLayer::kOpaque; i < (int)RenderLayer::kMaxCount; ++i) {
    auto instance_buffer = make_unique<UploadBuffer<InstanceData>>(device, max_instance_count, false);
    InstanceBufferGroups.push_back(move(instance_buffer));
  }
  //InstanceBuffer = std::make_unique<UploadBuffer<InstanceData>>(device, max_instance_count, false);
  //  WavesVB = std::make_unique<UploadBuffer<Vertex>>(device, wave_vertex_count, false);
}


FrameResource::~FrameResource()
{
}

}
