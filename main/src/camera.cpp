#include "camera.h"
#include "graphics_core.h"

namespace GameCore {

Camera MainCamera;
void InitMainCamera() {
  MainCamera.SetLens(0.25f*MathHelper::Pi, static_cast<float>(Graphics::Core.Width())/Graphics::Core.Height(), 1.0f, 1000.0f);

  XMFLOAT3 center(0.0f, 4.0f, -12.0f);
  XMFLOAT3 worldUp(0.0f, 1.0f, 0.0f);
  XMFLOAT3 target(0.0f, 0.0f, 0.0f);
  MainCamera.LookAt(center, target, worldUp);
  MainCamera.UpdateViewMatrix();
}

void Camera::SetLens(float fov_y, float aspect, float z_near, float z_far) {
  this->fov_y_ = fov_y;
  this->aspect_ = aspect;
  this->z_near_ = z_near;
  this->z_far_ = z_far;

  XMMATRIX proj = XMMatrixPerspectiveFovLH(0.25f * MathHelper::Pi, aspect, z_near, z_far);
  XMStoreFloat4x4(&proj_, proj);
}

void Camera::UpdateViewMatrix() {
  if (view_dirty_) {
    auto look = XMLoadFloat3(&look_);
    auto right = XMLoadFloat3(&right_);
    auto up = XMLoadFloat3(&up_);
    auto position = XMLoadFloat3(&position_);

    look = XMVector3Normalize(look);
    up = XMVector3Normalize(XMVector3Cross(look, right));
    right = XMVector3Cross(up, look);

    float x = -XMVectorGetX(XMVector3Dot(position, right));
    float y = -XMVectorGetX(XMVector3Dot(position, up));
    float z = -XMVectorGetX(XMVector3Dot(position, look));

    XMStoreFloat3(&right_, right);
    XMStoreFloat3(&up_, up);
    XMStoreFloat3(&look_, look);

    view_(0, 0) = right_.x;
    view_(1, 0) = right_.y;
    view_(2, 0) = right_.z;
    view_(3, 0) = x;

    view_(0, 1) = up_.x;
    view_(1, 1) = up_.y;
    view_(2, 1) = up_.z;
    view_(3, 1) = y;

    view_(0, 2) = look_.x;
    view_(1, 2) = look_.y;
    view_(2, 2) = look_.z;
    view_(3, 2) = z;

    view_(0, 3) = 0.0f;
    view_(1, 3) = 0.0f;
    view_(2, 3) = 0.0f;
    view_(3, 3) = 1.0f;

    view_dirty_ = false;
  }
 
}

void Camera::LookAt(FXMVECTOR pos, FXMVECTOR target, FXMVECTOR worldUp)
{
  XMVECTOR L = XMVector3Normalize(XMVectorSubtract(target, pos));
  XMVECTOR R = XMVector3Normalize(XMVector3Cross(worldUp, L));
  XMVECTOR U = XMVector3Cross(L, R);

  XMStoreFloat3(&position_, pos);
  XMStoreFloat3(&look_, L);
  XMStoreFloat3(&right_, R);
  XMStoreFloat3(&up_, U);

  view_dirty_ = true;
}

void Camera::LookAt(const XMFLOAT3& pos, const XMFLOAT3& target, const XMFLOAT3& up)
{
  XMVECTOR P = XMLoadFloat3(&pos);
  XMVECTOR T = XMLoadFloat3(&target);
  XMVECTOR U = XMLoadFloat3(&up);

  LookAt(P, T, U);

  view_dirty_ = true;
}


}