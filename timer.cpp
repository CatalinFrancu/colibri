#include <sys/time.h>
#include "timer.h"

Timer::Timer() {
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
