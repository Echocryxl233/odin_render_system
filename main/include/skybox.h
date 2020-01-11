#pragma once

#ifndef SKYBOX_H
#define SKYBOX_H

#include "utility.h"
#include "color_buffer.h"
#include "model.h"
#include "command_context.h"
#include "pipeline_state.h"
#include "texture_manager.h"

namespace Graphics {

namespace SkyBox {

void Initialize();
void Render(ColorBuffer& display_plane);

extern Texture* SkyBoxTexture;


}

};

#endif // !SKYBOX_H
