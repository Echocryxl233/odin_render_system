#pragma once

#include "light.h"
#include "utility.h"

using namespace Lighting;
using namespace DirectX;

struct Vertex
{
  XMFLOAT3 Pos;
  XMFLOAT3 Normal;
  XMFLOAT2 TexCoord;
};

__declspec(align(16)) 
struct ObjectConstants
{
  XMFLOAT4X4 World = MathHelper::Identity4x4();
};

  __declspec(align(16)) 
struct PassConstant {
  XMFLOAT4X4 View;
  XMFLOAT4X4 InvView;
  XMFLOAT4X4 Proj;
  XMFLOAT4X4 InvProj;
  XMFLOAT4X4 ViewProj;
  XMFLOAT4X4 ViewProjTex;
  XMFLOAT3 EyePosition;
  float padding;
  XMFLOAT4 AmbientLight = { 0.0f, 0.0f, 0.0f, 1.0f };


  Light Lights[1];
};

__declspec(align(16))
struct Material {
  XMFLOAT4 DiffuseAlbedo;
  XMFLOAT3 FresnelR0;
  float Roughness;
  XMFLOAT4X4 MatTransform;
};
//
//struct TempTexture {
//  std::string Name;
//  std::wstring Filename;
//  Microsoft::WRL::ComPtr<ID3D12Resource> Resource = nullptr;
//  Microsoft::WRL::ComPtr<ID3D12Resource> UploadHeap = nullptr;
//  D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHandle;
//};

