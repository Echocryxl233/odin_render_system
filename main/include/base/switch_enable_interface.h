#pragma once

#ifndef ENABLESWITCH_INTERFACE_H
#define ENABLESWITCH_INTERFACE_H

#include "game/game_timer.h"

using namespace GameCore;

class SwitchEnableInterface {
 public:
  bool Enabled() const { return is_enabled_; };
  void SetEnabled(bool enable) ;
 
 protected:
  SwitchEnableInterface() 
      : is_enabled_(true), timer(0.0f)
      {
  };
  virtual ~SwitchEnableInterface() { }

  virtual void OnSetEnabled(bool enable) {};
  bool is_enabled_; 

 private:
  float timer;
};

inline void SwitchEnableInterface::SetEnabled(bool enable) {
  const float delay = 0.5f;
  if (timer + delay > MainTimer.TotalTime())
    return;

  if (is_enabled_ != enable) {
    timer = MainTimer.TotalTime();
    is_enabled_ = enable;
    OnSetEnabled(enable);
  }
}


#endif // !ENABLESWITCH_INTERFACE_H

