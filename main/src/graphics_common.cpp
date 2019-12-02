#include "graphics_common.h"

namespace Graphics {
  CD3DX12_DEPTH_STENCIL_DESC DepthStateDisabled;
};

void Graphics::InitializeCommonState() {
  DepthStateDisabled = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
  DepthStateDisabled.DepthEnable = false;
  DepthStateDisabled.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;

}