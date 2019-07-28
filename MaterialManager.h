#pragma once

#ifndef MATERIALMANAGER_H
#define MATERIALMANAGER_H


#include "Material.h"

namespace OdinRenderSystem {

class MaterialManager  {
 public : 

  inline static MaterialManager* GetInstance() {
    static MaterialManager* instance = new MaterialManager();
    return instance;
  }

  Material* CreateMaterial(string name) ;
  Material* GetMaterial(string name);

  inline int MaterialCount() {
    return (int)(materials_.size());
  }

  inline map<string, unique_ptr<Material>>::iterator Begin() {
    return materials_.begin();
  }

  inline map<string, unique_ptr<Material>>::iterator End() {
    return materials_.end();
  }

 private :
  map<string, unique_ptr<Material>> materials_;
};    

}

#endif // !MATERIALMANAGER_H#pragma once
