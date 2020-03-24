#pragma once

#ifndef ENABLESWITCH_INTERFACE_H
#define ENABLESWITCH_INTERFACE_H

class SwitchEnableInterface {
 public:
  bool Enabled() const { return is_enabled_; };
  void SetEnabled(bool enable) ;
 
 protected:
  SwitchEnableInterface() {};
  virtual ~SwitchEnableInterface() { }
  bool is_enabled_; 
};

inline void SwitchEnableInterface::SetEnabled(bool enable) {
  if (is_enabled_ != enable)
    is_enabled_ = enable;
}


#endif // !ENABLESWITCH_INTERFACE_H

