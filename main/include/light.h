#pragma once

#ifndef LIGHT_H
#define LIGHT_H

#include "utility.h"

namespace Lighting {

using namespace DirectX;

struct Light
{
  XMFLOAT3 Strength;
  float FalloffStart;
  XMFLOAT3 Direction;
  float FalloffEnd;
  XMFLOAT3 Position;
  float SpotPower;
};

}

#endif // !LIGHT_H