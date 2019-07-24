#pragma once

#ifndef RENDERSYSTEM_H
#define RENDERSYSTEM_H



#include "Application.h"
#include "UploadBuffer.h"

#include "MeshGeometry.h"
#include "Material.h"
#include "RenderItem.h"
#include "FrameResource.h"
#include "Waves.h"
#include "BlurFilter.h"

#include "Common/Camera.h"
#include "CubeRenderTarget.h"
#include "ShadowMap.h"

using namespace std;
using namespace DirectX;
using Microsoft::WRL::ComPtr;

namespace OdinRenderSystem {

class RenderSystem : public OdinRenderSystem::Application
{
 public:
   RenderSystem(HINSTANCE h_instance);
   ~RenderSystem();

   virtual bool Initialize() override;
 protected:
  void DrawRenderItems() ;

  void DrawSceneToCubeMap();
  void DrawSceneToShadowMap();

  float GetHillsHeight(float x, float z) const;
  XMFLOAT3 GetHillsNormal(float x, float z) const;

  std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> GetStaticSamplers();
  void Pick(int sx, int sy);
  void CreateRtvAndDsvDescriptorHeaps() override;

  void BuildCubeDepthStencil();
  void UpdateCubeMapFacePassCBs();

 protected:
   void Update(const GameTimer& gt) override;
   void Draw(const GameTimer& gt) override;
   void OnKeyboardInput(const GameTimer& gt);


   void UpdateSkullAnimate(const GameTimer& gt);
   virtual void UpdateCamera(const GameTimer& gt);
   virtual void UpdateObjectCBs(const GameTimer& gt) ;
   virtual void UpdateMainPassCB(const GameTimer& gt);
   virtual void UpdateCubeMapFacePassCBs(const GameTimer& gt);
   // virtual void UpdateWave(const GameTimer& gt);
   virtual void UpdateInstanceData(const GameTimer& gt);
   virtual void UpdateMaterialBuffer(const GameTimer& gt);
   virtual void AnimateMaterials(const GameTimer& gt);

   virtual void UpdateReflectedPassCB(const GameTimer& gt);

   void DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems);
   void DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const int render_layer);

   //void DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems, );

   void OnResize() override;

   virtual void OnMouseDown(WPARAM btnState, int x, int y) override;
	 virtual void OnMouseUp(WPARAM btnState, int x, int y)  override;
	 virtual void OnMouseMove(WPARAM btnState, int x, int y) override;

   

 protected:
   void BuildShadersAndInputLayout();
   void BuildRootSignature();
   void BuildPSOs();
   void BuildLandGeometry();
   void BuildWavesGeometry();
   void BuildRenderItems();
   void BuildFrameResource();
   void BuildShapeGeometry();

   void BuildSkullGeometry();
   void BuildRoomGeometry();
   // --------  
   void BuildDescriptorHeaps();
   void BuildMaterial();
   void LoadTexture();
   
   void BuildPostProcessRootSignature();

   void BuildCubeFaceCamera(float x, float y, float z);

private:
  //  ComPtr<ID3D12DescriptorHeap> cbv_heap_;

  ComPtr<ID3DBlob> vertex_shader_code_;
  ComPtr<ID3DBlob> pixel_shader_code_;
  std::unordered_map<std::string, ComPtr<ID3DBlob>> shaders_;

  std::vector<D3D12_INPUT_ELEMENT_DESC> input_layout_;

  unique_ptr<UploadBuffer<ObjectConstants>> const_buffer_;

  UINT index_count_;

  unique_ptr<MeshGeometry> box_mesh_;
  unique_ptr<MeshGeometry> pyramid_mesh_;

  unique_ptr<MeshGeometry> box_mesh_2_;


  ComPtr<ID3D12RootSignature> root_signature_;
  //ComPtr<ID3D12PipelineState> pipeline_state_object_;
  ComPtr<ID3D12RootSignature> post_process_root_signature_;

  //  ComPtr<ID3D12DescriptorHeap> cbv_srv_uav_descriptor_heap = nullptr;


  unordered_map<string, ComPtr<ID3D12PipelineState>> pipeline_state_objects_;


  float mTheta = 1.5f*XM_PI;
  float mPhi = XM_PIDIV4;
  float mRadius = 5.0f;

  XMFLOAT4X4 mWorld = MathHelper::Identity4x4();
  XMFLOAT4X4 mView = MathHelper::Identity4x4();
  XMFLOAT4X4 mProj = MathHelper::Identity4x4();

  PassConstants main_pass_cb_;
  PassConstants mReflectedPassCB;

  POINT last_mouse_pos;

  unordered_map<string, unique_ptr<Material> > materials_;
  unordered_map<string, unique_ptr<MeshGeometry> > mesh_geos_;
  unordered_map<string, unique_ptr<Texture> > textures_;

  vector< unique_ptr<RenderItem> > render_items_;
  //  vector <RenderItem*> opaque_items_;

  vector <RenderItem*> items_layers_[(int)RenderLayer::kMaxCount];

  vector<unique_ptr<FrameResource>> frame_resources_;
  FrameResource* current_frame_resource_;
  int frame_resource_index_ = 0;

  UINT pass_cbv_offset_;

  unique_ptr<Waves> waves_;
  RenderItem* waves_render_item_;

  XMFLOAT3 eye_position_ = {0.0f, 0.0f, 0.0f};

  float mSunTheta = 1.25f*XM_PI;
	float mSunPhi = XM_PIDIV4;

  ComPtr<ID3D12DescriptorHeap> srv_descriptor_heap_ = nullptr;  //  use for srv_uav_cbv_heap
  ComPtr<ID3D12Resource> cube_depth_stencil_buffer_;

  RenderItem* skull_render_item_ = nullptr;
	RenderItem* mReflectedSkullRitem = nullptr;
	RenderItem* mShadowedSkullRitem = nullptr;
  std::string pso_name_transparent = "Transparent";

  std::string pso_name_reflection = "StencilReflection";
  std::string pso_name_mirror = "StencilMirror";

  std::unique_ptr<BlurFilter> blur_filter_;

  Camera camera_;
  Camera cube_map_cameras_[6];

  BoundingFrustum camera_frustum_;

  UINT instance_count_;

  bool enable_frustum_culling_;

  bool use_mouse_ = false;

  RenderItem* picked_item_;

  InstanceData* picked_instance_ = nullptr;

  unique_ptr<CubeRenderTarget> dynamic_cube_map_ = nullptr;

  CD3DX12_CPU_DESCRIPTOR_HANDLE cube_dsv_;

  int sky_heap_index_;
  int dynamic_heap_index_;

  unique_ptr<ShadowMap> shadow_map_;

  int shadow_heap_index;
  int mNullCubeSrvIndex;
  int mNullTexSrvIndex;

  CD3DX12_GPU_DESCRIPTOR_HANDLE null_cube_srv_handle_gpu;
};

}

#endif // !RENDERSYSTEM_H






