#pragma once

#ifndef RENDEROBJECT_H
#define RENDEROBJECT_H

#include "command_context.h"
#include "game_timer.h"
#include "model.h"
#include "utility.h"

using namespace GameCore;

class RenderObject {
 public:
  void LoadFromFile(const string& filename);
  void CreateSphere(float radius, std::uint32_t sliceCount, std::uint32_t stackCount);
  void CreateGrid(float width, float depth, std::uint32_t m, std::uint32_t n);

  Model& GetModel() { return model_;}
  ObjectConstants& GetObjConst() { return constants_; }

  XMFLOAT3& Position() { return position_; }

  void Update(const GameTimer& gt);

  void Render(GraphicsContext& context);

 private:
  Model model_;
  ObjectConstants constants_;
  XMFLOAT3 position_;
};

//inline void RenderObject::Position(float x, float y, float z) {
//  position_ = {x, y, z};
//}

#endif // !RENDEROBJECT_H



