#pragma once

#ifndef RENDERSYSTEM_H
#define RENDERSYSTEM_H

#include "gpu_buffer.h"
#include "model.h"
#include "pipeline_state.h"
#include "ssao.h"
#include "texture_manager.h"

#include "utility.h"

using namespace DirectX;

class RenderSystem {

 public:
  void OnMouseDown(WPARAM btnState, int x, int y) ;
  void OnMouseUp(WPARAM btnState, int x, int y) ;
  void OnMouseMove(WPARAM btnState, int x, int y) ;

  
  void Initialize();   

  void RenderScene();
  void Update();

 private:
  void RenderObjects();

  void LoadModel();
  void BuildPso(); 
  void UpdateCamera();

  void BuildDebugPlane(float x, float y, float w, float h, float depth);
  void BuildDebugPso();

 private:

  ByteAddressBuffer index_buffer_;
  StructuredBuffer vertex_buffer_;

  RootSignature root_signature_;
  GraphicsPso   graphics_pso_;
  
  XMFLOAT4X4 mWorld = MathHelper::Identity4x4();
  XMFLOAT4X4 mView = MathHelper::Identity4x4();
  XMFLOAT4X4 mProj = MathHelper::Identity4x4();

  float mTheta = 1.5f*XM_PI;
  float mPhi = XM_PIDIV4;
  float mRadius = 5.0f;

  float mSunTheta = 1.25f * XM_PI;
  float mSunPhi = XM_PIDIV4;

  ObjectConstants objConstants;
  PassConstant    pass_constant_;
  Material car_material_;

  //  unique_ptr<TempTexture> car_texture_;

  XMFLOAT3 eye_pos_;

  POINT mLastMousePos;

  Ssao ssao_;

//  -------------  for debug plane
  ByteAddressBuffer debug_index_buffer_;
  StructuredBuffer debug_vertex_buffer_;
  RootSignature debug_signature_;
  GraphicsPso   debug_pso_;

  //  unique_ptr<TempTexture> skull_texture_;

  Texture* car_texture_2_;
  Texture* skull_texture_2_;
    
};

#endif // !RENDERSYSTEM_H

