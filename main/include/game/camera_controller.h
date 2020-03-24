#pragma once

#ifndef CAMERACONTROLLER_H
#define CAMERACONTROLLER_H

#include "game/camera.h"
#include "game/game_timer.h"

namespace GameCore {

class CameraController {
 public:
  CameraController(Camera& target, XMFLOAT3 up);
  void Walk(float distance);
  void Strafe(float distance);

  void Pitch(float angle);
  void RotateY(float angle);

  void Update(const GameTimer& timer);

 private:
  Camera& target_camera_;
};


}



#endif // !CAMERACONTROLLER_H

