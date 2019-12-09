#pragma once

#ifndef OPTIONALSYSTEM_H
#define OPTIONALSYSTEM_H

#include "command_context.h"

#include "pipeline_state.h"
#include "render_queue.h"
#include "sampler_manager.h"

namespace Graphics {

class OptionalSystem {
 public :
  void virtual Initialize() = 0;
  void virtual Render(GraphicsContext& context);
  void SetRenderQueue(RenderQueue* queue);

  
 protected :
  virtual void OnRender(GraphicsContext& context) = 0;

 protected :
  RenderQueue* render_queue_;
  RootSignature root_signature_;
  GraphicsPso   graphics_pso_;
};

class ForwardShading : public OptionalSystem {
 public:
  ForwardShading() {};
  void Initialize() override;
  void OnRender(GraphicsContext& context) override;

};

class DeferredShading : public OptionalSystem {
 public :
  void Initialize() override;
  void OnRender(GraphicsContext& context) override;


 private:
 // void BuildQuan(float x, float y, float w, float h, float depth);
  void InitializeQuad();
 
 private:
  const static int kMRTBufferCount = 4;

  RootSignature pass1_signature_;
  GraphicsPso   pass1_pso_;

  RootSignature pass2_signature_;
  GraphicsPso   pass2_pso_;

  StructuredBuffer vertex_buffer_;
  ByteAddressBuffer index_buffer_;
};

};

#endif // !OPTIONALSYSTEM_H

