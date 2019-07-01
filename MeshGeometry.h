#pragma once

#ifndef MESHGEOMETRY_H
#define MESHGEOMETRY_H

#include "Util.h"

using namespace std;
using Microsoft::WRL::ComPtr;

namespace OdinRenderSystem {

struct SubmeshGeometry
{
  UINT StartIndexLocation = 0;
  UINT IndexCount = 0;

  INT BaseVertexLocation = 0;

  DirectX::BoundingBox Bounds;
};

struct MeshGeometry
{
  std::string Name;

  ComPtr<ID3D12Resource> VertexBufferUpload;
  ComPtr<ID3D12Resource> VertexBufferGpu;
  ComPtr<ID3DBlob> VertexBufferCpu;

  ComPtr<ID3D12Resource> IndexBufferUpload;
  ComPtr<ID3D12Resource> IndexBufferGpu;
  ComPtr<ID3DBlob> IndexBufferCpu;

  UINT VertexBufferSize = 0;
  UINT VertexByteStride = 0;
  UINT IndexBufferSize = 0;
  //UINT IndexCount = 0;
  DXGI_FORMAT IndexFormat = DXGI_FORMAT::DXGI_FORMAT_R16_UINT;

  unordered_map<string, SubmeshGeometry> DrawArgs;
  

  D3D12_VERTEX_BUFFER_VIEW VertexBufferView() const {
    D3D12_VERTEX_BUFFER_VIEW vbv;
    vbv.SizeInBytes = VertexBufferSize;
    vbv.StrideInBytes = VertexByteStride;
    vbv.BufferLocation = VertexBufferGpu->GetGPUVirtualAddress();
    return vbv;
  }

  D3D12_INDEX_BUFFER_VIEW IndexBufferView() const {
    D3D12_INDEX_BUFFER_VIEW ibv;
    ibv.SizeInBytes = IndexBufferSize;
    ibv.Format = DXGI_FORMAT::DXGI_FORMAT_R16_UINT;
    ibv.BufferLocation = IndexBufferGpu->GetGPUVirtualAddress();
    return ibv;
  }

};


}


#endif // !MESHGEOMETRY_H



