#pragma once

#ifndef SSR_H
#define SSR_H


#include "color_buffer.h"
#include "depth_stencil_buffer.h"
#include "pipeline_state.h"
#include "render_object.h"
#include "utility.h"

namespace GI {

namespace Specular {

class SSR {
 public:

  void Initialize(uint32_t width, uint32_t height);
  void OnResize(uint32_t width, uint32_t height);

  void DrawNormalAndDepth();
  void RenderReflection();

  ColorBuffer& NormalMap() { return normal_map_; }
  ColorBuffer& ColorMap() { return color_map_; }
  DepthStencilBuffer& DepthMap() { return depth_buffer_; }

 private:
  void CreatePso();

 private:
  ColorBuffer normal_map_;
  ColorBuffer color_map_;
  DepthStencilBuffer depth_buffer_;

  RootSignature signature_;
  GraphicsPso pso_;
  
  uint32_t width_;
  uint32_t height_;
};

extern SSR MainSSR;

};

};

#endif // !SSR_H

