#ifndef __TIMER_H__
#define __TIMER_H__

#include "defines.h"

/**
 * A millisecond timer.
 */

class Timer {

  u64 startTime;

public:

  /**
   * Instantiates and starts the timer.
   */
  Timer();

  /**
   * Resets the timer to 0.
   */
  void reset();

  /**
   * @return The number of milliseconds elapsed since the last reset.
   */
  u64 get();

private:

  /**
   * @return the UNIX timestamp in milliseconds.
   */
  u64 getTimestamp();

};

#endif
