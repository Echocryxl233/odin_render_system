#include "game/camera_controller.h"

using namespace GameCore;

CameraController::CameraController(Camera& target, XMFLOAT3 up) 
    : target_camera_(target) {

}

void CameraController::Walk(float distance) {
  //  position += distance * look_
  XMVECTOR s = XMVectorReplicate(distance);
  XMVECTOR look = XMLoadFloat3(&target_camera_.look_);
  XMVECTOR position = XMLoadFloat3(&target_camera_.position_);

  XMStoreFloat3(&target_camera_.position_, XMVectorMultiplyAdd(s, look, position));

  target_camera_.view_dirty_ = true;
} 

void CameraController::Strafe(float distance) {
  XMVECTOR s = XMVectorReplicate(distance);
  XMVECTOR right = XMLoadFloat3(&target_camera_.right_);
  XMVECTOR position = XMLoadFloat3(&target_camera_.position_);

  XMStoreFloat3(&target_camera_.position_, XMVectorMultiplyAdd(s, right, position));

  target_camera_.view_dirty_ = true;
}

void CameraController::Pitch(float angle) {
  auto pitch_matrix = XMMatrixRotationAxis(XMLoadFloat3(&target_camera_.right_), angle);

  XMStoreFloat3(&target_camera_.up_,  XMVector3TransformNormal(XMLoadFloat3(&target_camera_.up_), pitch_matrix));
  XMStoreFloat3(&target_camera_.look_,  XMVector3TransformNormal(XMLoadFloat3(&target_camera_.look_), pitch_matrix));
  target_camera_.view_dirty_ = true;
}

void CameraController::RotateY(float angle) {
  auto rotation_matrix = XMMatrixRotationY(angle);

  XMStoreFloat3(&target_camera_.right_,  XMVector3TransformNormal(XMLoadFloat3(&target_camera_.right_), rotation_matrix));
  XMStoreFloat3(&target_camera_.up_,  XMVector3TransformNormal(XMLoadFloat3(&target_camera_.up_), rotation_matrix));
  XMStoreFloat3(&target_camera_.look_,  XMVector3TransformNormal(XMLoadFloat3(&target_camera_.look_), rotation_matrix));
  target_camera_.view_dirty_ = true;
}

void CameraController::Update(const GameTimer& gt) {
  const float dt = gt.DeltaTime();

  if (GetAsyncKeyState('W') & 0x8000)
    Walk(10.0f * dt);
    //  target_camera_.Walk(10.0f * dt);

  if (GetAsyncKeyState('S') & 0x8000)
    Walk(-10.0f * dt);

  if (GetAsyncKeyState('A') & 0x8000)
    Strafe(-10.0f * dt);

  if (GetAsyncKeyState('D') & 0x8000)
    Strafe(10.0f * dt);

  target_camera_.UpdateViewMatrix();
}
 

