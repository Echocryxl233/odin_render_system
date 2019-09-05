#pragma once

#ifndef TEXTUREMANAGER_H
#define TEXTUREMANAGER_H

#include "utility.h"
#include "gpu_resource.h"

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
  void CreateDeriveView();

 private:
  wstring filename_;
  Microsoft::WRL::ComPtr<ID3D12Resource> upload_heap_ = nullptr;
  D3D12_CPU_DESCRIPTOR_HANDLE srv_handle_;
};

class TextureManager {
 public:
  static TextureManager& Instance() {
    static TextureManager instance;
    return instance;
  }

 Texture* RequestTexture(wstring filename);

 private:
  map<wstring, Texture*> texture_pool_;
  
};



#endif // !TEXTUREMANAGER_H

