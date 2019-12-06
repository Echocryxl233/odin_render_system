#pragma once

#include "light.h"
#include "utility.h"
#include "gpu_buffer.h"

using namespace Lighting;
using namespace DirectX;
using namespace std;

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
  XMFLOAT4X4 InvViewProj;
  XMFLOAT4X4 ViewProjTex;
  XMFLOAT3 EyePosition;
  float padding;
  float ZNear;
  float ZFar;
  float padding1;
  float padding2;
  XMFLOAT4 AmbientLight = { 0.0f, 0.0f, 0.0f, 1.0f };

  Light Lights[kPointLightCount];
};

__declspec(align(16))
struct Material {
  XMFLOAT4 DiffuseAlbedo;
  XMFLOAT3 FresnelR0;
  float Roughness;
  XMFLOAT4X4 MatTransform;
};

class MeshManager;
class Model;

class Mesh {
friend class MeshManager;
friend class Model;
 public:
  StructuredBuffer& VertexBuffer() { return vertex_buffer_; }
  ByteAddressBuffer& IndexBuffer() { return index_buffer_; }

  void LoadFromObj(const string& filename);

 private:
  void LoadMesh(const string& filename);

  StructuredBuffer vertex_buffer_;
  ByteAddressBuffer index_buffer_;
};

class MeshManager {
 public:
  static MeshManager& Instance() {
    static MeshManager instance;
    return instance;
  }

  Mesh* LoadFromFile(const string& filename);

private:
  map<string, Mesh*> mesh_pool_;
};

class Model {
 public:
  void LoadFromFile(const string& filename);

  Mesh* GetMesh() const { return mesh_; }
  Material& GetMaterial() { return material_; }
  /*vector<D3D12_CPU_DESCRIPTOR_HANDLE>& TextureSrvs { return SRVs; }*/

  D3D12_CPU_DESCRIPTOR_HANDLE* TextureData() { return SRVs.data(); }
  int32_t TextureCount() { return (int32_t)SRVs.size(); }

 private:
  string name;
  Mesh* mesh_;

  XMFLOAT4X4 world_ = MathHelper::Identity4x4();
  XMFLOAT3 position_;

  Material material_;
  //  Texture* textures[];
  vector<D3D12_CPU_DESCRIPTOR_HANDLE> SRVs;  
};

