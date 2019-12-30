#pragma once

#ifndef SKYBOX_H
#define SKYBOX_H

#include "utility.h"
#include "color_buffer.h"
#include "model.h"
#include "command_context.h"
#include "pipeline_state.h"

namespace Graphics {

namespace SkyBox {

void Initialize();
void Render(GraphicsContext& context, ColorBuffer& display_plane);

}

};

#endif // !SKYBOX_H
