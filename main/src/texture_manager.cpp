#include "texture_manager.h"

#include "graphics/command_context.h"
#include "graphics/graphics_core.h"

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

void CubeTexture::CreateDeriveView() {
  assert(resource_ != nullptr);

  D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
  srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  srv_desc.Format = resource_->GetDesc().Format;
  srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
  srv_desc.TextureCube.MostDetailedMip = 0;
  srv_desc.TextureCube.MipLevels = resource_->GetDesc().MipLevels;
  srv_desc.TextureCube.ResourceMinLODClamp = 0.0f;

  srv_handle_ = DescriptorAllocatorManager::Instance().Allocate(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

  Graphics::Core.Device()->CreateShaderResourceView(resource_.Get(), &srv_desc, srv_handle_);
}


Texture* TextureManager::RequestTexture(const wstring& filename, D3D12_SRV_DIMENSION type) {
  auto iter = texture_pool_.find(filename);

  if (iter != texture_pool_.end()) {
    return iter->second;
  } 

  Texture* texture; // = new Texture();
  switch (type)
  {
  case D3D12_SRV_DIMENSION_UNKNOWN:
    break;
  case D3D12_SRV_DIMENSION_BUFFER:
    break;
  case D3D12_SRV_DIMENSION_TEXTURE1D:
    break;
  case D3D12_SRV_DIMENSION_TEXTURE1DARRAY:
    break;
  case D3D12_SRV_DIMENSION_TEXTURE2D: 
    texture = new Texture();
    break;
  case D3D12_SRV_DIMENSION_TEXTURE2DARRAY:
    break;
  case D3D12_SRV_DIMENSION_TEXTURE2DMS:
    break;
  case D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY:
    break;
  case D3D12_SRV_DIMENSION_TEXTURE3D:
    break;
  case D3D12_SRV_DIMENSION_TEXTURECUBE:
    texture = new CubeTexture();
    break;
  case D3D12_SRV_DIMENSION_TEXTURECUBEARRAY:
    break;
  case D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE:
    break;
  default:
    break;
  }

  texture->Create(filename);
  texture_pool_.emplace(filename, texture);

  return texture_pool_[filename];
}