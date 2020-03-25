#pragma once

#ifndef TEXTUREMANAGER_H
#define TEXTUREMANAGER_H

#include "resource/gpu_resource.h"
#include "utility.h"

using namespace std;

class TextureManager;

class Texture : public GpuResource {
friend class TextureManager;

 public:
  ~Texture() {
    Destroy();
  }

  D3D12_CPU_DESCRIPTOR_HANDLE& SrvHandle() { return srv_handle_; }

 protected:
  void Create(wstring filename);
  virtual void CreateDeriveView();

 protected:
  D3D12_CPU_DESCRIPTOR_HANDLE srv_handle_;

 private:
  wstring filename_;
  Microsoft::WRL::ComPtr<ID3D12Resource> upload_heap_ = nullptr;

};

class CubeTexture : public Texture {
 protected:
  void CreateDeriveView();
};

class TextureManager : public Singleton<TextureManager> {
friend class Singleton<TextureManager>;

 public:

 Texture* RequestTexture(const wstring& filename, D3D12_SRV_DIMENSION type = D3D12_SRV_DIMENSION_TEXTURE2D);

 private:
  TextureManager() = default;

 private:
  map<wstring, Texture*> texture_pool_;
  
};



#endif // !TEXTUREMANAGER_H

