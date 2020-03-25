#pragma once
#ifndef POSTPROCESS_BASE_H
#define POSTPROCESS_BASE_H

#include "utility.h"
#include "resource/color_buffer.h"

namespace PostProcess {

class PostProcessBase : public SwitchEnableInterface {
 public :
  virtual ~PostProcessBase();
  void Render(ColorBuffer& input);

 protected:
  PostProcessBase() = default;
  virtual void OnRender(ColorBuffer& input) = 0;
};

}

#endif // !POSTPROCESS_BASE_H

