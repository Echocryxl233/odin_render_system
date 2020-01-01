#pragma once

#ifndef GRAPHICSCOMMON_H
#define GRAPHICSCOMMON_H

#include "utility.h"
#include "model.h"
#include "gpu_resource.h"
#include "mesh_geometry.h"
#include "render_queue.h"
#include "pipeline_state.h"

namespace Graphics {

extern CD3DX12_DEPTH_STENCIL_DESC DepthStateDisabled;
extern PassConstant MainConstants;

extern D3D12_BLEND_DESC BlendDisable;
extern D3D12_BLEND_DESC BlendAdditional;
extern RenderQueue MainQueue;

void InitializeCommonState();
void InitializeLights();
void InitRenderQueue();
void UpdateConstants();

};

#endif // !GRAPHICSCOMMON_H
