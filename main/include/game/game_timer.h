#pragma once

#ifndef GAMETIME_H
#define GAMETIME_H

namespace GameCore {

class GameTimer {
 public:
  GameTimer();

  float DeltaTime() const;
  float TotalTime() const;

  void Reset();
  void Tick();
  void Stop();
  void Start();

 private:

  double delta_time_;
  double second_per_count_;

  __int64 base_time_;
  __int64 curr_time_;
  __int64 prev_time_;
  __int64 stop_time_;
  __int64 pause_time_;


  //  __int64 

  bool stopped_;
};

extern GameTimer MainTimer;

}




#endif // !GAMETIME_H

