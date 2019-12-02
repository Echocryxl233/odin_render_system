#include "render_object.h"

void RenderObject::LoadFromFile(const string& filename) {
  model_.LoadFromFile(filename);
  position_ = {0.0f, 0.0f, 0.0f};
  constants_.World = MathHelper::Identity4x4();
}

void RenderObject::Update(const GameTimer& gt) {
  auto scaling = XMMatrixScaling(0.7f, 0.7f, 0.7f);
  auto offset = XMMatrixTranslation(position_.x, position_.y, position_.z);
  auto transform = scaling * offset;

  //  XMStoreFloat4x4(&constants_.World , );
  //  XMMATRIX world = XMLoadFloat4x4(&constants_.World);
  XMStoreFloat4x4(&constants_.World, XMMatrixTranspose(transform)); 
}