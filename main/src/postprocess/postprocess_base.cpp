#include "postprocess/postprocess_base.h"

namespace PostProcess {

PostProcessBase::~PostProcessBase() {

}

void PostProcessBase::Render(ColorBuffer& input) {
  if (!is_enabled_)
    return;
  
  OnRender(input);
}


};

