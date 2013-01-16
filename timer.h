#ifndef __TIMER_H__
#define __TIMER_H__
#include "defines.h"

/* Resets the millisecond timer to 0 */
void timerReset();

/* Gets the number of milliseconds elapsed since the last timerReset() */
u64 timerGet();

#endif

