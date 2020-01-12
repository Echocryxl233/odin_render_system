#pragma once

#ifndef GRAPHICSUTILITY_H
#define GRAPHICSUTILITY_H

#include "color_buffer.h"

namespace Graphics {

namespace Utility {

void InitBlur();

void Blur(ColorBuffer& input, int blur_count);

};

};

#endif // !GRAPHICSUTILITY_H

