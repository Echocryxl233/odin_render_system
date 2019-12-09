#pragma once

#ifndef RENDERQUEUE_H
#define RENDERQUEUE_H

#include "utility.h"
#include "render_object.h"

namespace Graphics {

//class OptionalSystem;
//class ForwardShaing;

class RenderQueue {
//friend class OptionalSystem;
//friend class ForwardShading;

 public:
  void AddObject(RenderObject* object);
  void Clear();
  void Update(GameTimer& gt);

  vector<RenderObject*>::const_iterator Begin() { return queue_.begin(); }
  vector<RenderObject*>::const_iterator End() { return queue_.end(); }
  

 private:
  vector<RenderObject*> queue_;
};

extern RenderQueue MainRenderQueue;

};

#endif // !RENDERQUEUE_H
