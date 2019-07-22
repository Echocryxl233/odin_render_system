#pragma once
#ifndef FRAMERESOURCE_H
#define FRAMERESOURCE_H

#include "Util.h"
#include "UploadBuffer.h"
#include "Material.h"
#include "Light.h"

using namespace DirectX;
using namespace std;

namespace OdinRenderSystem {

enum class RenderLayer : int
{
  kOpaque = 0,
  kMirrors,
  kReflected,
  kTransparent,
  kShadow,
  kSky,
  kMaxCount
};


struct Vertex {

  Vertex() = default;
  Vertex(float x, float y, float z, float nx, float ny, float nz, float u, float v) :
		Pos(x, y, z),
		Normal(nx, ny, nz),
		TexCoord(u, v) {}
  XMFLOAT3 Pos;
  //  XMFLOAT4 Color;
  XMFLOAT3 Normal;
  XMFLOAT2 TexCoord;
};

struct ObjectConstants
{
    //  XMFLOAT4X4 WorldViewProj = MathHelper::Identity4x4();
  XMFLOAT4X4 World;
  DirectX::XMFLOAT4X4 TexTransform = MathHelper::Identity4x4();
  UINT     MaterialIndex;
	UINT     ObjPad0;
	UINT     ObjPad1;
	UINT     ObjPad2;
};

struct InstanceData
{
    //  XMFLOAT4X4 WorldViewProj = MathHelper::Identity4x4();
  XMFLOAT4X4 World;
  XMFLOAT4X4 TexTransform = MathHelper::Identity4x4();
  UINT MaterialIndex;
	UINT InstancePad0;
	UINT InstancePad1;
	UINT InstancePad2;
};

struct MaterialData {
  DirectX::XMFLOAT4 DiffuseAlbedo = { 1.0f, 1.0f, 1.0f, 1.0f };
	DirectX::XMFLOAT3 FresnelR0 = { 0.01f, 0.01f, 0.01f };
	float Roughness = 64.0f;

	// Used in texture mapping.
	DirectX::XMFLOAT4X4 MatTransform = MathHelper::Identity4x4();

	UINT DiffuseMapIndex = 0;
  UINT NormalMapIndex = 0;
	UINT MaterialPad0;
	UINT MaterialPad1;
	UINT MaterialPad2;
};

struct PassConstants
{
  DirectX::XMFLOAT4X4 View = MathHelper::Identity4x4();
  DirectX::XMFLOAT4X4 InvView = MathHelper::Identity4x4();
  DirectX::XMFLOAT4X4 Proj = MathHelper::Identity4x4();
  DirectX::XMFLOAT4X4 InvProj = MathHelper::Identity4x4();
  DirectX::XMFLOAT4X4 ViewProj = MathHelper::Identity4x4();
  DirectX::XMFLOAT4X4 InvViewProj = MathHelper::Identity4x4();
  DirectX::XMFLOAT3 EyePosW = { 0.0f, 0.0f, 0.0f };
  float cbPerObjectPad1 = 0.0f;
  DirectX::XMFLOAT2 RenderTargetSize = { 0.0f, 0.0f };
  DirectX::XMFLOAT2 InvRenderTargetSize = { 0.0f, 0.0f };
  float NearZ = 0.0f;
  float FarZ = 0.0f;
  float TotalTime = 0.0f;
  float DeltaTime = 0.0f;

  DirectX::XMFLOAT4 AmbientLight = { 0.0f, 0.0f, 0.0f, 1.0f };

  Light Lights[MaxLights];
};

struct FrameResource
{
 public:
  FrameResource(ID3D12Device* device, UINT pass_CB_count, UINT object_CB_count, UINT max_instance_count,
    UINT material_count, UINT waves_vertex_count);
  FrameResource(const FrameResource& rhs) = delete;
  FrameResource& operator=(const FrameResource& rhs) = delete;
  ~FrameResource();

  Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CmdListAlloc;

  std::unique_ptr<UploadBuffer<ObjectConstants>> ObjectCB = nullptr;
  std::unique_ptr<UploadBuffer<PassConstants>> PassCB = nullptr;

  //  std::unique_ptr<UploadBuffer<InstanceData>> InstanceBuffer = nullptr;

  vector<unique_ptr<UploadBuffer<InstanceData>>> InstanceBufferGroups;

  std::unique_ptr<UploadBuffer<MaterialData>> MaterialBuffer = nullptr;

  //  std::unique_ptr<UploadBuffer<Vertex>> WavesVB = nullptr;

  UINT64 Fence = 0;
};

};


#endif // !FRAMERESOURCE_H




