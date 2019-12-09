#pragma once

#ifndef GRAPHICSCOMMON_H
#define GRAPHICSCOMMON_H

#include "utility.h"
#include "model.h"
#include "gpu_resource.h"
#include "mesh_geometry.h"

namespace Graphics {

extern CD3DX12_DEPTH_STENCIL_DESC DepthStateDisabled;
extern PassConstant MainConstants;

void InitializeCommonState();
void InitializeLights();
void InitRenderQueue();
void UpdateConstants();
void CreateQuad(float x, float y, float w, float h, float depth, 
    GpuBuffer& vertex_buffer, GpuBuffer& index_buffer);

};

#endif // !GRAPHICSCOMMON_H
