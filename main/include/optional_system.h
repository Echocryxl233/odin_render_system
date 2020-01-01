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
  virtual ~OptionalSystem() {};

  virtual void Initialize() = 0;
  virtual void Render(GraphicsContext& context);
  virtual void OnResize(uint32_t width, uint32_t height) {};

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

class DeferredBase : public OptionalSystem {
 public :
  void Initialize() override;
 protected : 
  //StructuredBuffer vertex_buffer_;
  //ByteAddressBuffer index_buffer_;
  Mesh mesh_quad_;
};

class DeferredShading : public DeferredBase {
 public :
  void Initialize() override;
  void OnRender(GraphicsContext& context) override;
 
 private:
  const static int kMRTBufferCount = 4;

  RootSignature pass1_signature_;
  GraphicsPso   pass1_pso_;

  RootSignature pass2_signature_;
  GraphicsPso   pass2_pso_;


};

class DeferredLighting : public DeferredBase {
 public:
  void Initialize() override;
  void OnRender(GraphicsContext& context) override;
  void OnResize(uint32_t width, uint32_t height) override;

 private:
  const static int kMRTBufferCount = 4;

  RootSignature pass1_signature_;
  GraphicsPso   pass1_pso_;

  RootSignature pass2_signature_;
  GraphicsPso   pass2_pso_;

  RootSignature pass3_signature_;
  GraphicsPso   pass3_pso_;

  RootSignature pass4_signature_;
  GraphicsPso   pass4_pso_;

  ColorBuffer normal_buffer_;
  ColorBuffer color_buffer_;
};

};

#endif // !OPTIONALSYSTEM_H

