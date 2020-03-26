#pragma once

#ifndef GRAPHICSUTILITY_H
#define GRAPHICSUTILITY_H

#include "graphics/command_context.h"
#include "resource/color_buffer.h"

namespace Graphics {

namespace Utility {

void InitBlur();

void Blur(ComputeContext& compute_context, ColorBuffer& input, int blur_count);

};

};

#endif // !GRAPHICSUTILITY_H

