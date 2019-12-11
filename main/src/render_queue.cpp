#include "game_timer.h"
#include "render_queue.h"
#include "utility.h"

namespace Graphics {
//  RenderQueue MainRenderQueue;
//  RenderQueue TransparentQueue;

RenderGroup::RenderGroup(GroupType type) 
    : group_type_(type) {

}

void RenderGroup::AddObject(RenderObject* object) {
  group_.push_back(object);
}

void RenderGroup::Clear() {
  group_.clear();
}

void RenderGroup::Update(GameTimer& gt) {
  for(auto& object : group_) {
    object->Update(gt);
  }
}

void RenderQueue::AddGroup(GroupType id, RenderGroup* group) {
  auto it = group_map_.find(id);
  if (it == group_map_.end()) {
    group_map_.emplace(id, group);
  }
}

RenderGroup* RenderQueue::GetGroup(GroupType id) {
  auto it = group_map_.find(id);
  if (it != group_map_.end()) {
    return it->second;
  }
  return nullptr;
}

void RenderQueue::Clear() {
  group_map_.clear();

}

void RenderQueue::Update(GameTimer& gt) {
  for (auto it : group_map_) {
    it.second->Update(gt);
  }
}

}