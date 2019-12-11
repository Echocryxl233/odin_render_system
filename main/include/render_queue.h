#pragma once

#ifndef RENDERQUEUE_H
#define RENDERQUEUE_H

#include "utility.h"
#include "render_object.h"

namespace Graphics {

namespace RenderGroupType {
  enum RenderGroupType {
    kOpaque,
    kOpaque2,
    kTransparent
  };
}

using GroupType = RenderGroupType::RenderGroupType;

class RenderGroup {
public:
  RenderGroup(GroupType type);
  void AddObject(RenderObject* object);
  void Clear();
  void Update(GameTimer& gt);
  GroupType GetType() const { return group_type_; }

  vector<RenderObject*>::const_iterator Begin() { return group_.begin(); }
  vector<RenderObject*>::const_iterator End() { return group_.end(); }
 private:
  GroupType group_type_;
  vector<RenderObject*> group_;
};

class RenderQueue {


 public:
  void AddGroup(GroupType, RenderGroup* group);
  RenderGroup* GetGroup(GroupType);

  void Clear();
  void Update(GameTimer& gt);

  map<GroupType, RenderGroup*>::const_iterator Begin() { return group_map_.begin(); }
  map<GroupType, RenderGroup*>::const_iterator End() { return group_map_.end(); }



 private:
  map<GroupType, RenderGroup*> group_map_;
};

//  extern RenderQueue MainRenderQueue;
//  extern RenderQueue TransparentQueue;

};

#endif // !RENDERQUEUE_H
