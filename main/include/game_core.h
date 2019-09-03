#pragma once

#ifndef GAMECORE_H
#define GAMECORE_H

#include "render_system.h"
#include "graphics_core.h"
#include "utility.h"

namespace GameCore {

class GameApp {

};
  
extern long client_width_ ;
extern long client_height_;

void Run(RenderSystem& rs);

};

#endif // !GAMECORE_H

