#include "render_object.h"
#include "graphics_common.h"

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

void RenderObject::Render(GraphicsContext& context) {
  context.SetDynamicConstantBufferView(0, sizeof(constants_), &constants_);
  context.SetDynamicConstantBufferView(1, sizeof(model_.GetMaterial()), &model_.GetMaterial());
  context.SetDynamicConstantBufferView(2, sizeof(Graphics::MainConstants), &Graphics::MainConstants);

  context.SetVertexBuffer(model_.GetMesh()->VertexBuffer().VertexBufferView());
  context.SetIndexBuffer(model_.GetMesh()->IndexBuffer().IndexBufferView());

  context.SetDynamicDescriptors(3, 0, model_.TextureCount(), model_.TextureData());
  context.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  context.DrawIndexedInstanced(model_.GetMesh()->IndexBuffer().ElementCount(), 1, 0, 0, 0);
}