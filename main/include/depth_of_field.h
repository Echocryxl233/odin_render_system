#pragma once

#ifndef FIELDOFVIEW_H
#define FIELDOFVIEW_H

#include "color_buffer.h"
#include "command_context.h"
#include "pipeline_state.h"
#include "root_signature.h"
#include "utility.h"

namespace PostProcess {

class DepthOfField {
 public:
  void Initialize();
  void OnResize(UINT width, UINT height);

  void Render(ColorBuffer& input, int blur_count);
  ColorBuffer& BlurBuffer() { return blur_buffer_0_; }

 private:
  vector<float> CalculateGaussWeights(float sigma);

 private:
  void Blur(ColorBuffer& input, int blur_count);
 private:
  ColorBuffer blur_buffer_0_;
  ColorBuffer blur_buffer_1_;
  DepthStencilBuffer depth_buffer_;

  RootSignature root_signature_;
  ComputePso horizontal_pso_;
  ComputePso vertical_pso_;

  UINT width_;
  UINT height_;
};

extern DepthOfField DoF;

};

#endif // !FIELDOFVIEW_H

