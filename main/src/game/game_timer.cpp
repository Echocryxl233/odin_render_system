#include <windows.h>

#include "game/game_timer.h"

using namespace GameCore;

namespace GameCore {
  GameTimer MainTimer;
}

GameTimer::GameTimer() 
  : base_time_(0), curr_time_(0), prev_time_(0), 
   pause_time_(0), stop_time_(0), delta_time_(0), stopped_(false) {
  __int64 count_per_second;

  QueryPerformanceFrequency((LARGE_INTEGER*)&count_per_second);
  second_per_count_ = 1.0 / count_per_second;
}

float GameTimer::DeltaTime() const {
  return (float)delta_time_;
}

float GameTimer::TotalTime() const {
  __int64 head_time = stopped_ ? stop_time_ : curr_time_;

  return (float)(((head_time - pause_time_) - base_time_ ) * second_per_count_);

}

void GameTimer::Reset() {
  __int64 curr_time;
  QueryPerformanceCounter((LARGE_INTEGER*)&curr_time);

  base_time_ = curr_time;
  prev_time_ = curr_time;
  stop_time_ = 0;
}

void GameTimer::Start() {
  __int64 startTime;
  QueryPerformanceCounter((LARGE_INTEGER*)&startTime);

  if (stopped_) {
   

    //  __int64 curr_time;
    //  QueryPerformanceCounter((LARGE_INTEGER*)&curr_time);
    pause_time_ += (startTime - stop_time_);
    prev_time_ = startTime;
    stop_time_ = 0;
    stopped_ = false;
  }
}

void GameTimer::Stop() {
  if (!stopped_) {
    stopped_ = true;

    __int64 curr_time;
    QueryPerformanceCounter((LARGE_INTEGER*)&curr_time);
    stop_time_ = curr_time;
  }
}

void GameTimer::Tick() {
  if (stopped_) {
    delta_time_ = 0;
    return;
  }
  
  QueryPerformanceCounter((LARGE_INTEGER*)&curr_time_);
  delta_time_ = (curr_time_ - prev_time_) * second_per_count_;
  prev_time_ = curr_time_;

  if (delta_time_ < 0)
      delta_time_ = 0;

}

