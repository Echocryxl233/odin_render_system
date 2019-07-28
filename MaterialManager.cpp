#include "MaterialManager.h"

namespace OdinRenderSystem {

  Material* MaterialManager::CreateMaterial(string name) {
    //    std::cout << name << endl;
    auto material = GetMaterial(name);
    if (material != nullptr)
      return material;

    auto mat = std::make_unique<Material>();
    int index = (int)(materials_.size());
    mat->Name = name;
    mat->MatCBIndex = index;
    materials_[name] = move(mat);
    return materials_[name].get();
  }


  Material* MaterialManager::GetMaterial(string name) {
    auto it = materials_.find(name);

    if (it != materials_.end())
      return it->second.get();

    return nullptr;
  }

}