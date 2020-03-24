#pragma once

#include "light.h"
#include "utility.h"
#include "resource/gpu_buffer.h"
#include "mesh_geometry.h"

using namespace Lighting;
using namespace DirectX;
using namespace std;
using namespace Graphics::Geometry;

struct Vertex
{
  XMFLOAT3 Position;
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
  XMFLOAT4X4 ShadowTransform;
  XMFLOAT3 EyePosition;
  float padding;
  float ZNear;
  float ZFar;
  float padding1;
  float padding2;
  XMFLOAT4 AmbientLight = { 0.0f, 0.0f, 0.0f, 1.0f };

  Light Lights[kDirectightCount + kPointLightCount];
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

  //  void CreateSphere(float radius, std::uint32_t sliceCount, std::uint32_t stackCount);
  void CreateQuad(float x, float y, float w, float h, float depth);

 private:
  void LoadMesh(const string& filename);

  void LoadMeshData(const GeometryGenerator::MeshData& data);

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
  Mesh* CreateSphere(float radius, std::uint32_t sliceCount, std::uint32_t stackCount);
  Mesh* CreateGrid(float width, float depth, std::uint32_t m, std::uint32_t n);

private:
  map<string, Mesh*> mesh_pool_;
};

class Model {
 public:
  void LoadFromFile(const string& filename);

  void CreateSphere(float radius, std::uint32_t sliceCount, std::uint32_t stackCount);
  void CreateGrid(float width, float depth, std::uint32_t m, std::uint32_t n);

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

