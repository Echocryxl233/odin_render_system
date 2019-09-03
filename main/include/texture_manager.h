#pragma once

#ifndef TEXTUREMANAGER_H
#define TEXTUREMANAGER_H

#include "utility.h"
#include "gpu_resource.h"

using namespace std;

class Texture : public GpuResource {
 public:
  ~Texture() {
    Destroy();
  }

 private:
  void CreateFromMemory();
};

//class TextureManager {
// public:
//  
//};

#endif // !TEXTUREMANAGER_H

