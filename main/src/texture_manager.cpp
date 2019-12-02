#include "texture_manager.h"

#include "command_context.h"
#include "graphics_core.h"

void Texture::Create(wstring name) {
  filename_ = name;
  GraphicsContext& texture_context = GraphicsContext::Begin(L"Texture Context");
  ASSERT_SUCCESSED(DirectX::CreateDDSTextureFromFile12(Graphics::Core.Device(),
    texture_context.GetCommandList(), filename_.c_str(),
    resource_, upload_heap_));

  texture_context.Finish(true);

  CreateDeriveView();
}

void Texture::CreateDeriveView() {
  assert(resource_ != nullptr);

  D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
  srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  srv_desc.Format = resource_->GetDesc().Format;
  srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
  srv_desc.Texture2D.MostDetailedMip = 0;
  srv_desc.Texture2D.MipLevels = resource_->GetDesc().MipLevels;
  srv_desc.Texture2D.ResourceMinLODClamp = 0.0f;

  srv_handle_ = DescriptorAllocatorManager::Instance().Allocate(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

  Graphics::Core.Device()->CreateShaderResourceView(resource_.Get(), &srv_desc, srv_handle_);
}

Texture* TextureManager::RequestTexture(const wstring& filename) {
  auto iter = texture_pool_.find(filename);

  if (iter != texture_pool_.end()) {
    return iter->second;
  } 

  Texture* texture = new Texture();
  texture->Create(filename);
  texture_pool_.emplace(filename, texture);

  return texture_pool_[filename];
}