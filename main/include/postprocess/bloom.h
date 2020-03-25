#pragma once

#ifndef BLOOM_H
#define BLOOM_H

#include "graphics/pipeline_state.h"
#include "resource/color_buffer.h"
#include "postprocess_base.h"
#include "base/singleton.h"

namespace PostProcess {

class Bloom : public PostProcessBase , public Singleton<Bloom> {
friend class Singleton<Bloom>;
  
 public:
  Bloom() = default;
  ~Bloom();
  void Initialize();
  //void Render(ColorBuffer& input);

  ColorBuffer& BloomBuffer() { return bloom_buffer_; }

 protected:
  void OnRender(ColorBuffer& input);

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

extern Bloom* BloomEffect;

};

#endif // !BLOOM_H

