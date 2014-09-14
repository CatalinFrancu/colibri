#ifndef ZOBRIST_H
#define ZOBRIST_H

/* One random number for every piece at every square. */
extern u64 zrBoard[64][2][KING + 1];

/* One random number for the side to move (set when Black is to move) */
extern u64 zrSide;

/* One random number for each file to indicate the en passant square (if any) */
extern u64 zrEp[8];

/* Initializes a set of random 64-bit Zobrist numbers. */
void zobristInit();

/* Returns the zobrist key of a board. */
u64 getZobrist(Board *b);

/* Updates the Zobrist key for a Board b on which we're making Move m */
u64 updateZobrist(u64 z, Board *b, Move m);

#endif
