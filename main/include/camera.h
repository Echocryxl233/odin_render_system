#pragma once

#ifndef CAMERA_H
#define CAMERA_H

#include "utility.h"

namespace GameCore {

using namespace DirectX;

class Camera {

friend class CameraController;

 public:
  float ZNear() const;
  float ZFar() const;
  float Aspect() const;

  XMFLOAT4X4 View4x4f() const;
  XMFLOAT4X4 Proj4x4f() const;

  XMMATRIX View() const;
  XMMATRIX Proj() const;

  XMFLOAT3 Position() const ;
  
  void LookAt(FXMVECTOR pos, FXMVECTOR target, FXMVECTOR worldUp);
  void LookAt(const XMFLOAT3& pos, const XMFLOAT3& target, const XMFLOAT3& up);
  
  void UpdateViewMatrix();
  void SetLens(float fov_y, float aspect, float z_near, float z_far);

 private:
  bool view_dirty_= true;

  float z_near_ = 0.0f;
  float z_far_ = 0.0f;
  float aspect_ = 0.0f;
  float fov_y_ = 0.0f;
  
  XMFLOAT3 position_;
  XMFLOAT3 up_;
  XMFLOAT3 right_;
  XMFLOAT3 look_;

  XMFLOAT4X4 view_ = MathHelper::Identity4x4();
  XMFLOAT4X4 proj_ = MathHelper::Identity4x4();
  
};

inline float Camera::ZNear() const {
  return z_near_;
}

inline float Camera::ZFar() const {
  return z_far_;
}

inline float Camera::Aspect() const {
  return aspect_;
}

inline XMFLOAT4X4 Camera::View4x4f() const {
  return view_;
}

inline XMFLOAT4X4 Camera::Proj4x4f() const {
  return proj_;
}

inline XMMATRIX Camera::View() const {
  return XMLoadFloat4x4(&view_);
}

inline XMMATRIX Camera::Proj() const {
  return XMLoadFloat4x4(&proj_);
}

inline XMFLOAT3 Camera::Position() const {
  return position_;
}

extern Camera MainCamera;

void InitMainCamera();

}

#endif // !CAMERA_H

