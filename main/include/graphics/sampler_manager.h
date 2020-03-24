#pragma once

#ifndef SAMPLERMANAGER_H
#define SAMPLERMANAGER_H

#include <d3d12.h>
#include "color.h"

class SamplerDesc : public D3D12_SAMPLER_DESC {
 public : 
  SamplerDesc() {
    Filter = D3D12_FILTER_ANISOTROPIC;
    AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    MipLODBias = 0.0f;
    MaxAnisotropy = 16;
    ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    BorderColor[0] = 1.0f;
    BorderColor[1] = 1.0f;
    BorderColor[2] = 1.0f;
    BorderColor[3] = 1.0f;
    MinLOD = 0.0f;
    MaxLOD = D3D12_FLOAT32_MAX;
  }

  D3D12_CPU_DESCRIPTOR_HANDLE CreateDescriptor();

  void SetTextureAddressMode(D3D12_TEXTURE_ADDRESS_MODE mode) {
    AddressU = mode;
    AddressV = mode;
    AddressW = mode;
  }
 
  void SetBoarderColor(Color color) {
    BorderColor[0] = color.R();
    BorderColor[1] = color.G();
    BorderColor[2] = color.B();
    BorderColor[3] = color.A();
  }

 private:


};

#endif // !SAMPLERMANAGER_H

