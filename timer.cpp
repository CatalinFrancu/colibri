#include <sys/time.h>
#include "timer.h"

Timer::Timer(u64 turnback) {
  this->turnback = turnback;
  reset();
}

void Timer::reset() {
  startTime = getTimestamp();
}

u64 Timer::get() {
  return getTimestamp() - startTime;
}

u64 Timer::getTimestamp() {
  timeval tv;
  gettimeofday(&tv, NULL);
  return 1000ull * tv.tv_sec + tv.tv_usec / 1000;
}

bool Timer::ticked() {
  u64 now = getTimestamp();
  if (now - startTime >= turnback) {
    startTime += turnback;
    return true;
  } else {
    return false;
  }
}
