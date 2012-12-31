#ifndef __BITMANIP_H__
#define __BITMANIP_H__
#include "defines.h"

/* Returns the number of set bits */
byte popCount(u64 x);

/* Returns the index of the last set bit. When x only has one set bit, this works as a logarithm. */
byte ctz(u64 x);

/* Reverse the 8 bytes of x */
u64 reverseBytes(u64 x);

/* Flip and/or rotate a bitboard */
u64 rotate(u64 x, int orientation);

#endif
