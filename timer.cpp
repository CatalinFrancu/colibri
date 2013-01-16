#include <sys/time.h>
#include "timer.h"

u64 timerMillisec;

void timerReset() {
  timeval tv;
  gettimeofday(&tv, NULL);
  timerMillisec = 1000ull * tv.tv_sec + tv.tv_usec / 1000;
}

u64 timerGet() {
  timeval tv;
  gettimeofday(&tv, NULL);
  u64 now = 1000ull * tv.tv_sec+ tv.tv_usec / 1000;
  return now - timerMillisec;
}
