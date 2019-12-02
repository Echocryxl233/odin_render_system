#pragma once

#ifndef RENDERSYSTEM_H
#define RENDERSYSTEM_H


#include "model.h"
#include "pipeline_state.h"
#include "render_object.h"
#include "ssao.h"
#include "texture_manager.h"

#include "utility.h"

using namespace DirectX;

class RenderSystem {

 public:
  int width;
  int height;
  
  void Initialize();   
  void OnResize(UINT width, UINT height) {
    this->width = width;
    this->height = height;
    ssao_.Resize(width, height);
  }

  void RenderObjects(GraphicsContext& context);
  void RenderScene();
  void Update();

 private:

  void RenderSingleObject(GraphicsContext& context, RenderObject& object);

  void InitializeLights();
  void InitializeRenderQueue();

  void ForwardRender(GraphicsContext& context);
  void DeferredRender(GraphicsContext& context);
  

  //  void LoadModel();
  void BuildPso(); 
  void BuildDeferredShadingPso();

  void BuildDebugPlane(float x, float y, float w, float h, float depth);
  void BuildDebugPso();

 private:

  RootSignature root_signature_;
  GraphicsPso   graphics_pso_;

// -------------------- for deferred shading
  RootSignature deferred_pass1_signature_;
  GraphicsPso   deferred_pass1_pso_;

  RootSignature deferred_pass2_signature_;
  GraphicsPso   deferred_pass2_pso_;
//  -------------------- end
  

  XMFLOAT4X4 mView = MathHelper::Identity4x4();

  float mTheta = 1.5f*XM_PI;
  float mPhi = XM_PIDIV4;
  float mRadius = 5.0f;

  float mSunTheta = 1.25f * XM_PI;
  float mSunPhi = XM_PI; //  XM_PIDIV4

  //ObjectConstants objConstants;
  PassConstant    pass_constant_;
  //  Material car_material_;

  //  unique_ptr<TempTexture> car_texture_;

  XMFLOAT3 eye_pos_;

  POINT mLastMousePos;

  Ssao ssao_;

//  -------------  for debug plane
  ByteAddressBuffer debug_index_buffer_;
  StructuredBuffer debug_vertex_buffer_;
  RootSignature debug_signature_;
  GraphicsPso   debug_pso_;
//  ----------------  end debug plane

//  

  //  unique_ptr<TempTexture> skull_texture_;

  Texture* ice_texture_2_;
  Texture* grass_texture_2_;

  Model car_;
  Model skull_;

  Mesh mesh_;

  RenderObject render_object_;

  vector<RenderObject> render_queue_;

  const XMMATRIX kTextureTransform = {
    0.5f, 0.0f, 0.0f, 0.0f,
    0.0f, -0.5f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.5f, 0.5f, 0.0f, 1.0f,
  };

  bool use_deferred = false;
  float timer = 0.0f;
};

#endif // !RENDERSYSTEM_H

