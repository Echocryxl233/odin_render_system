#pragma once

#ifndef MATERIAL_H
#define MATERIAL_H

#include "Util.h"

using namespace std;

namespace OdinRenderSystem {

struct MaterialConstants {

  /*float Roughness;
  DirectX::XMFLOAT4 DiffuseAlbedo = {1.0f, 1.0f,1.0f,1.0f};
  DirectX::XMFLOAT3 FresnelR0 = {1.0f, 1.0f,1.0f};

  DirectX::XMFLOAT4X4 MatTransform = MathHelper::Identity4x4();*/

  DirectX::XMFLOAT4 DiffuseAlbedo = { 1.0f, 1.0f, 1.0f, 1.0f };
	DirectX::XMFLOAT3 FresnelR0 = { 0.01f, 0.01f, 0.01f };
	float Roughness = 0.25f;

	// Used in texture mapping.
	DirectX::XMFLOAT4X4 MatTransform = MathHelper::Identity4x4();
};

struct Material
{
  std::string Name;

  int MatCBIndex = 0;
  int DiffuseSrvHeapIndex = 0;

  // Dirty flag indicating the material has changed and we need to
  // update the constant buffer. Because we have a material constant
  // buffer for each FrameResource, we have to apply the update to each 
  // FrameResource. Thus, when we modify a material we should set
  // NumFramesDirty = gNumFrameResources so that each frame resource
  // gets the update.
  int NumFrameDirty = kNumFrameResources;

  float Roughness;
  DirectX::XMFLOAT4 DiffuseAlbedo = {1.0f, 1.0f,1.0f,1.0f};
  DirectX::XMFLOAT3 FresnelR0 = {1.0f, 1.0f,1.0f};

  DirectX::XMFLOAT4X4 MatTransform = MathHelper::Identity4x4();
};

struct Texture {
  std::string Name;
  std::wstring Filename;

  Microsoft::WRL::ComPtr<ID3D12Resource> Resource;
  Microsoft::WRL::ComPtr<ID3D12Resource> UploadHeap;
};

}




#endif // !MATERIAL_H

