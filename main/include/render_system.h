#pragma once

#ifndef RENDERSYSTEM_H
#define RENDERSYSTEM_H


#include "model.h"
#include "optional_system.h"
#include "graphics/pipeline_state.h"
#include "render_object.h"
#include "GI/ssao.h"
#include "texture_manager.h"

#include "utility.h"

using namespace DirectX;
using namespace Odin;

class RenderSystem {

 public:
  
  void Initialize();   
  void OnResize(UINT width, UINT height) {
    //  ssao_.Resize(width, height);
    if (current_optional_system_ != nullptr) {
      current_optional_system_->OnResize(width, height);
    }
  }

  void Render();
  void Update();

  void PrevOptionalSystem();
  void NextOptionalSystem();

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

  D3D12_VIEWPORT screen_viewport_;
  D3D12_RECT scissor_rect_;
//  ----------------  end debug plane

  Odin::OptionalSystem* current_optional_system_;

  Odin::OptionalSystem* forward_shading_;
  Odin::OptionalSystem* deferred_shading_;
  Odin::OptionalSystem* deferred_lighting_;

  vector<Odin::OptionalSystem*> optional_systems_;
  int current_optional_system_index_;
  float select_timer_;

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

