#pragma once

#ifndef RENDERSYSTEM_H
#define RENDERSYSTEM_H


#include "model.h"
#include "optional_system.h"
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
    //  ssao_.Resize(width, height);
    if (optional_system_ != nullptr) {
      optional_system_->OnResize(width, height);
    }
  }

  void RenderScene();
  void Update();

 private:
 
  void BuildDebugPso();
  void DrawDebug(GraphicsContext& context);

 private:

  RootSignature root_signature_;
  GraphicsPso   graphics_pso_;

  float mTheta = 1.5f*XM_PI;
  float mPhi = XM_PIDIV4;
  float mRadius = 5.0f;

  float mSunTheta = 1.25f * XM_PI;
  float mSunPhi = XM_PI; //  XM_PIDIV4


  PassConstant    pass_constant_;

  POINT mLastMousePos;

  //Ssao ssao_;

//  -------------  for debug plane
  ByteAddressBuffer debug_index_buffer_;
  StructuredBuffer debug_vertex_buffer_;
  Mesh debug_mesh_;

  RootSignature debug_signature_;
  GraphicsPso   debug_pso_;
//  ----------------  end debug plane

  Graphics::OptionalSystem* optional_system_;

  Graphics::OptionalSystem* forward_shading_;
  Graphics::OptionalSystem* deferred_shading_;
  Graphics::OptionalSystem* deferred_lighting_;

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

