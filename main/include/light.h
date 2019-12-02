#pragma once

#ifndef LIGHT_H
#define LIGHT_H

#include "utility.h"

namespace Lighting {

using namespace DirectX;

struct Light
{
  XMFLOAT3 Strength;
  float FalloffStart; //  for point light
  XMFLOAT3 Direction;
  float FalloffEnd; //  for point light
  XMFLOAT3 Position;
  float SpotPower;  //  for spot light
};

}

#endif // !LIGHT_H