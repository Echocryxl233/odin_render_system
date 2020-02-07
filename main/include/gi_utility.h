#pragma once

#ifndef GIUTILITY_H
#define GIUTILITY_H

#include "utility.h"
#include "color_buffer.h"
#include "depth_stencil_buffer.h"
#include "command_context.h"
#include "render_object.h"

namespace GI {

const DXGI_FORMAT kNormalMapFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;

namespace Utility {

void Initialize();

void DrawNormalAndDepthMap(GraphicsContext& context, ColorBuffer& normal_map,   
    DepthStencilBuffer& depth_buffer, const vector<RenderObject*>& objects);

};

};

#endif // !GIUTILITY_H

