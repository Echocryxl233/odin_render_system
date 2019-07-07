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

using namespace std;
using namespace DirectX;
using Microsoft::WRL::ComPtr;

namespace OdinRenderSystem {

enum class RenderLayer : int
{
	kOpaque = 0,
	kMirrors ,
	kReflected,
	kTransparent,
	kShadow,
	Count
};


class RenderSystem : public OdinRenderSystem::Application
{
 public:
   RenderSystem(HINSTANCE h_instance);
   ~RenderSystem();

   virtual bool Initialize() override;
 protected:
  void DrawRenderItems() ;

  float GetHillsHeight(float x, float z) const;
  XMFLOAT3 GetHillsNormal(float x, float z) const;

  std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> GetStaticSamplers();

 protected:
   void Update(const GameTimer& gt) override;
   void Draw(const GameTimer& gt) override;
   void OnKeyboardInput(const GameTimer& gt);

   virtual void UpdateCamera(const GameTimer& gt);
   virtual void UpdateObjectCBs(const GameTimer& gt) ;
   virtual void UpdateMainPassCB(const GameTimer& gt);
   // virtual void UpdateWave(const GameTimer& gt);
   virtual void UpdateInstanceData(const GameTimer& gt);
   virtual void UpdateMaterialBuffer(const GameTimer& gt);
   virtual void AnimateMaterials(const GameTimer& gt);

   virtual void UpdateReflectedPassCB(const GameTimer& gt);

   void DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems);

   void OnResize() override;

   virtual void OnMouseDown(WPARAM btnState, int x, int y) override;
	 virtual void OnMouseUp(WPARAM btnState, int x, int y)  override;
	 virtual void OnMouseMove(WPARAM btnState, int x, int y) override;

   

 protected:
   void BuildShadersAndInputLayout();
   void BuildRootSignature();
   void BuildPipelineStateObjects();
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

  vector <RenderItem*> items_layers_[(int)RenderLayer::Count];

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

  RenderItem* mSkullRitem = nullptr;
	RenderItem* mReflectedSkullRitem = nullptr;
	RenderItem* mShadowedSkullRitem = nullptr;
  std::string pso_name_transparent = "Transparent";

  std::string pso_name_reflection = "StencilReflection";
  std::string pso_name_mirror = "StencilMirror";

  std::unique_ptr<BlurFilter> blur_filter_;

  Camera camera_;
  BoundingFrustum camera_frustum_;

  UINT instance_count_;

  bool enable_frustum_culling_;
};

}

#endif // !RENDERSYSTEM_H






