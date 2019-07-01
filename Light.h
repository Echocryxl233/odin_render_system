#pragma once

#ifndef LIGHT_H
#define LIGHT_H

#include "Util.h"

namespace OdinRenderSystem {

struct Light
{
  DirectX::XMFLOAT3 Strength;
  float FalloffStart;
  DirectX::XMFLOAT3 Direction;
  float FalloffEnd;
  DirectX::XMFLOAT3 Position;
  float SpotPower;
};


}


#endif // !LIGHT_H



