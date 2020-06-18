#ifndef __TIMER_H__
#define __TIMER_H__

#include "defines.h"

/**
 * A millisecond timer. It can be instantiated with a turnback argument, which
 * allows the caller to measure time in intervals.
 */

class Timer {

  u64 startTime;
  u64 turnback;

public:

  /**
   * Instantiates and starts a timer with no turnback ability.
   */
  Timer() : Timer(0) { }

  /**
   * Instantiates and starts a timer with the given turnback interval in milliseconds.
   */
  Timer(u64 turnback);

  /**
   * Resets the timer to 0.
   */
  void reset();

  /**
   * @return The number of milliseconds elapsed since the last reset.
   */
  u64 get();

  /**
   * If the timer indicates more than <turnback> milliseconds, turns it back
   * by turnback milliseconds and returns true. Otherwise leaves the timer
   * unchanged and returns false.
   */
  bool ticked();

private:

  /**
   * @return the UNIX timestamp in milliseconds.
   */
  u64 getTimestamp();

};

#endif
