#include "game_timer.h"
#include "render_queue.h"

namespace Graphics {
RenderQueue MainRenderQueue;

void RenderQueue::AddObject(RenderObject* object) {
  queue_.push_back(object);
}

void RenderQueue::Clear() {
  queue_.clear();
}

void RenderQueue::Update(GameTimer& gt) {
  for(auto& object : queue_) {
    object->Update(gt);
  }
}

}