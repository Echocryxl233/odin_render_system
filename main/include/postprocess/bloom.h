#pragma once

#ifndef BLOOM_H
#define BLOOM_H

#include "pipeline_state.h"
#include "color_buffer.h"

namespace PostProcess {

class Bloom {
 public:
  void Initialize();
  void Render(ColorBuffer& input);

  ColorBuffer& BloomBuffer() { return bloom_buffer_; }

 private:
  void ResizeBuffers(UINT width, UINT height, UINT mip_level, DXGI_FORMAT format);

 private:
  RootSignature root_signature_;
  ComputePso pso_;

  UINT width_;
  UINT height_;
  UINT mip_level_;
  DXGI_FORMAT format_;
  
  ColorBuffer bloom_buffer_;
  ColorBuffer temp_buffer_;

  //  RootSignature composite_root_signature_;
  ComputePso composite_pso_;
};


extern Bloom BloomEffect;

};

#endif // !BLOOM_H

