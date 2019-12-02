#pragma once

#ifndef RENDEROBJECT_H
#define RENDEROBJECT_H

#include "utility.h"
#include "model.h"
#include "game_timer.h"

using namespace GameCore;

class RenderObject {
 public:
  void LoadFromFile(const string& filename);

  Model& GetModel() { return model_;}
  ObjectConstants& GetObjConst() { return constants_; }

  XMFLOAT3& Position() { return position_; }

  //  void Position(float x, float y, float z) ;

  void Update(const GameTimer& gt);

 private:
  Model model_;
  ObjectConstants constants_;
  XMFLOAT3 position_;

};

//inline void RenderObject::Position(float x, float y, float z) {
//  position_ = {x, y, z};
//}

#endif // !RENDEROBJECT_H



