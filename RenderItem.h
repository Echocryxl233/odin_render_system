#pragma once

#ifndef RENDERITEM_H
#define RENDERITEM_H

#include "MeshGeometry.h"
#include "Util.h"
#include "RenderItem.h"
#include "FrameResource.h"

using namespace DirectX;

namespace OdinRenderSystem {

struct RenderItem {
  RenderItem() = default;

  XMFLOAT4X4 World =  MathHelper::Identity4x4();
  XMFLOAT4X4 TexTransform = MathHelper::Identity4x4();

  int NumFrameDirty = kNumFrameResources;

  int ObjectCBIndex = -1;

  MeshGeometry* Geo;
  Material* Mat = nullptr;

  D3D12_PRIMITIVE_TOPOLOGY PrimitiveType = D3D12_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

  BoundingBox Bounds;
  vector<InstanceData> Instances;

  UINT IndexCount = 0;
  UINT StartIndexLocation = 0;
  int BaseVertexLocation = 0;
};

}



#endif // !RENDERITEM_H

