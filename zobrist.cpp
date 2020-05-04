#include <stdlib.h>
#include "bitmanip.h"
#include "defines.h"
#include "zobrist.h"

u64 zrBoard[64][2][KING + 1];
u64 zrSide;
u64 zrEp[8];

u64 rand64() {
  return
    (((u64)rand() & 0xffffull) <<  0) ^
    (((u64)rand() & 0xffffull) << 16) ^
    (((u64)rand() & 0xffffull) << 32) ^
    (((u64)rand() & 0xffffull) << 48);
}

void zobristInit() {
  for (int sq = 0; sq < 64; sq++) {
    for (int side = 0; side < 2; side++) {
      for (int piece = PAWN; piece <= KING; piece ++) {
        zrBoard[sq][side][piece] = rand64();
      }
    }
  }
  zrSide = rand64();
  for (int i = 0; i < 8; i++) {
    zrEp[i] = rand64();
  }
}

u64 getZobrist(Board *b) {
  u64 z = 0;
  for (int side = WHITE; side <= BLACK; side++) {
    int index = (side == WHITE) ? BB_WP : BB_BP;
    for (int piece = PAWN; piece <= KING; piece++) {
      u64 mask = b->bb[index++];
      int sq;
      while (mask) {
        GET_BIT_AND_CLEAR(mask, sq);
        z ^= zrBoard[sq][side][piece];
      }
    }
  }
  if (b->side == BLACK) {
    z ^= zrSide;
  }
  if (b->bb[BB_EP]) {
    int sq = ctz(b->bb[BB_EP]);
    z ^= zrEp[sq & 7];
  }
  return z;
}

u64 updateZobrist(u64 z, Board *b, Move m) {
  int allMe = (b->side == WHITE) ? BB_WALL : BB_BALL;
  int allYou = BB_WALL + BB_BALL - allMe;
  u64 toMask = 1ull << m.to;

  // m.piece disappears from m.from.
  z ^= zrBoard[m.from][b->side][m.piece];

  // m.piece or m.promotion appears at m.to.
  if (m.promotion) {
    z ^= zrBoard[m.to][b->side][m.promotion];
  } else {
    z ^= zrBoard[m.to][b->side][m.piece];
  }

  // Any opponent's piece disappears from m.to.
  if (b->bb[allYou] & toMask) {
    int i = allYou + 1, p = PAWN;
    while (!(b->bb[i] & toMask)) {
      i++;
      p++;
    }
    z ^= zrBoard[m.to][1 - b->side][p];
  }

  // If the move is an en passant capture, remove the opponent's pawn.
  if ((m.piece == PAWN) && (toMask == b->bb[BB_EP])) {
    int captureSq = (b->side == WHITE) ? (m.to - 8) : (m.to + 8);
    z ^= zrBoard[captureSq][1 - b->side][PAWN];
  }

  // Clear the old en passant value, if it existed.
  if (b->bb[BB_EP]) {
    int sq = ctz(b->bb[BB_EP]);
    z ^= zrEp[sq & 7];
  }

  // Set the new en passant value, if it exists
  if ((m.piece == PAWN) && (abs(m.to - m.from) == 16)) {
    z ^= zrEp[m.from & 7];
  }

  z ^= zrSide;
  return z;
}
