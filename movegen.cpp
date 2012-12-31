#include <iostream>
#include "bitmanip.h"
#include "movegen.h"
#include "precomp.h"

/* Add a move at the end of a move array */
inline void pushMove(Move *m, int *numMoves, byte piece, byte fromSquare, byte toSquare, byte promotion) {
  m[*numMoves].piece = piece;
  m[*numMoves].from = fromSquare;
  m[*numMoves].to = toSquare;
  m[*numMoves].promotion = promotion;
  (*numMoves)++;
}

/* Get all legal knight moves (destMask = BB_EMPTY) or captures (destMask = BB_WALL / BB_BALL) */
inline void getKnightMoves(u64 pieceSet, u64 destMask, Move *m, int *numMoves) {
  u64 fromSq, toSq, mask;
  while (pieceSet) {
    GET_BIT_AND_CLEAR(pieceSet, fromSq);
    mask = knightAttacks[fromSq] & destMask;
    while (mask) {
      GET_BIT_AND_CLEAR(mask, toSq);
      pushMove(m, numMoves, KNIGHT, fromSq, toSq, 0);
    }
  }
}

/* Get all legal king moves (destMask = BB_EMPTY) or captures (destMask = BB_WALL / BB_BALL) */
inline void getKingMoves(u64 pieceSet, u64 destMask, Move *m, int *numMoves) {
  u64 fromSq, toSq, mask;
  while (pieceSet) {
    GET_BIT_AND_CLEAR(pieceSet, fromSq);
    mask = kingAttacks[fromSq] & destMask;
    while (mask) {
      GET_BIT_AND_CLEAR(mask, toSq);
      pushMove(m, numMoves, KING, fromSq, toSq, 0);
    }
  }
}

/* Get all legal rank moves / captures for queens and rooks */
inline void getRankMoves(int pieceType, bool capture, u64 pieceSet, u64 enemy, u64 empty, Move *m, int *numMoves) {
  u64 fromSq, toSq, mask;
  byte file, rank, rankOccupancy;

  while (pieceSet) {
    GET_BIT_AND_CLEAR(pieceSet, fromSq);
    file = fromSq & 7;
    rank = fromSq & ~7; /* 8 * rank, actually */
    rankOccupancy = ((~empty) >> (rank + 1)) & 63;
    mask = (u64)isoMove[file][rankOccupancy] << rank;
    mask &= capture ? enemy : empty;   /* These are all the legal destinations */
    while (mask) {
      GET_BIT_AND_CLEAR(mask, toSq);
      pushMove(m, numMoves, pieceType, fromSq, toSq, 0);
    }
  }
}

/* Get all legal file moves / captures for queens and rooks */
inline void getFileMoves(int pieceType, bool capture, u64 pieceSet, u64 enemy, u64 empty, Move *m, int *numMoves) {
  u64 fromSq, toSq, mask;
  byte file, rank, fileOccupancy, moves;

  while (pieceSet) {
    GET_BIT_AND_CLEAR(pieceSet, fromSq);
    file = fromSq & 7;
    rank = fromSq >> 3;
    // - Shift by file to bring the relevant bits on file A
    // - And with FILE_A to discard everything else
    // - Multiply with FILE_MAGIC to bring the relevant bits on the 8th rank
    // - Shift and and to capture the six relevant occupancy bits.
    fileOccupancy = (((((~empty) >> file) & FILE_A) * FILE_MAGIC) >> 57) & 63;
    moves = isoMove[7 - rank][fileOccupancy];
    mask = fileConfiguration[moves] << file;
    mask &= capture ? enemy : empty;   /* These are all the legal destinations */
    while (mask) {
      GET_BIT_AND_CLEAR(mask, toSq);
      pushMove(m, numMoves, pieceType, fromSq, toSq, 0);
    }
  }
}

/* Get all legal diagonal / antidiagonal moves / captures for queens and bishops */
inline void getDiagonalMoves(int pieceType, u64 *diagArray, bool capture, u64 pieceSet, u64 enemy, u64 empty, Move *m, int *numMoves) {
  u64 fromSq, toSq, mask;
  byte file, occupancy, moves;

  while (pieceSet) {
    GET_BIT_AND_CLEAR(pieceSet, fromSq);
    file = fromSq & 7;
    // - Isolate the diagonal
    // - Shift it up to the 8th rank
    // - Shift down to capture the relevant occupancy bits
    occupancy = ((((~empty) & diagArray[fromSq]) * DIAG_MAGIC) >> 57) & 63;

    // - Look up the possible moves
    // - Multiply it across all the columns
    // - Apply the diagonal mask again to isolate the diagonal
    moves = isoMove[file][occupancy];
    mask = (moves * DIAG_MAGIC) & diagArray[fromSq];
    mask &= capture ? enemy : empty;   /* These are all the legal destinations */
    while (mask) {
      GET_BIT_AND_CLEAR(mask, toSq);
      pushMove(m, numMoves, pieceType, fromSq, toSq, 0);
    }
  }
}

inline void getWhitePawnCaptures(Board *b, u64 ignoredFile, int shift, Move *m, int *numMoves) {
  u64 mask = ((b->bb[BB_WP] & ~ignoredFile) << shift) & (b->bb[BB_BALL] | b->bb[BB_EP]);
  byte fromSq, toSq;
  while (mask) {
    GET_BIT_AND_CLEAR(mask, toSq);
    fromSq = toSq - shift;
    if (toSq < 56) {
      pushMove(m, numMoves, PAWN, fromSq, toSq, 0);
    } else {
      for (byte prom = KNIGHT; prom <= KING; prom++) {
        pushMove(m, numMoves, PAWN, fromSq, toSq, prom);
      }
    }
  }
}

inline void getBlackPawnCaptures(Board *b, u64 ignoredFile, int shift, Move *m, int *numMoves) {
  u64 mask = ((b->bb[BB_BP] & ~ignoredFile) >> shift) & (b->bb[BB_WALL] | b->bb[BB_EP]);
  byte fromSq, toSq;
  while (mask) {
    GET_BIT_AND_CLEAR(mask, toSq);
    fromSq = toSq + shift;
    if (toSq >= 8) {
      pushMove(m, numMoves, PAWN, fromSq, toSq, 0);
    } else {
      for (byte prom = KNIGHT; prom <= KING; prom++) {
        pushMove(m, numMoves, PAWN, fromSq, toSq, prom);
      }
    }
  }
}

/* Get all legal non-capturing moves on the given board if White is to move */
int getWhiteNonCaptures(Board *b, Move *m) {
  int numMoves = 0, fromSq, toSq;
  u64 mask;

  // Pawn pushes (one-step)
  mask = (b->bb[BB_WP] << 8) & b->bb[BB_EMPTY];
  u64 oneStep = mask;
  while (mask) {
    GET_BIT_AND_CLEAR(mask, toSq);
    fromSq = toSq - 8;
    if (toSq < 56) {
      pushMove(m, &numMoves, PAWN, fromSq, toSq, 0);
    } else {
      for (byte prom = KNIGHT; prom <= KING; prom++) {
        pushMove(m, &numMoves, PAWN, fromSq, toSq, prom);
      }
    }
  }

  // Pawn pushes (two-step)
  mask = ((oneStep & RANK_3) << 8) & b->bb[BB_EMPTY];
  while (mask) {
    GET_BIT_AND_CLEAR(mask, toSq);
    pushMove(m, &numMoves, PAWN, toSq - 16, toSq, 0);
  }

  getKnightMoves(b->bb[BB_WN], b->bb[BB_EMPTY], m, &numMoves);
  getDiagonalMoves(BISHOP, diagonal, false, b->bb[BB_WB], 0, b->bb[BB_EMPTY], m, &numMoves);
  getDiagonalMoves(BISHOP, antidiagonal, false, b->bb[BB_WB], 0, b->bb[BB_EMPTY], m, &numMoves);
  getRankMoves(ROOK, false, b->bb[BB_WR], 0, b->bb[BB_EMPTY], m, &numMoves);
  getFileMoves(ROOK, false, b->bb[BB_WR], 0, b->bb[BB_EMPTY], m, &numMoves);
  getRankMoves(QUEEN, false, b->bb[BB_WQ], 0, b->bb[BB_EMPTY], m, &numMoves);
  getFileMoves(QUEEN, false, b->bb[BB_WQ], 0, b->bb[BB_EMPTY], m, &numMoves);
  getDiagonalMoves(QUEEN, diagonal, false, b->bb[BB_WQ], 0, b->bb[BB_EMPTY], m, &numMoves);
  getDiagonalMoves(QUEEN, antidiagonal, false, b->bb[BB_WQ], 0, b->bb[BB_EMPTY], m, &numMoves);
  getKingMoves(b->bb[BB_WK], b->bb[BB_EMPTY], m, &numMoves);
  return numMoves;
}

/* Get all legal captures on the given board if White is to move */
int getWhiteCaptures(Board* b, Move* m) {
  int numMoves = 0;
  getWhitePawnCaptures(b, FILE_H, 9, m, &numMoves);
  getWhitePawnCaptures(b, FILE_A, 7, m, &numMoves);
  getKnightMoves(b->bb[BB_WN], b->bb[BB_BALL], m, &numMoves);
  getDiagonalMoves(BISHOP, diagonal, true, b->bb[BB_WB], b->bb[BB_BALL], b->bb[BB_EMPTY], m, &numMoves);
  getDiagonalMoves(BISHOP, antidiagonal, true, b->bb[BB_WB], b->bb[BB_BALL], b->bb[BB_EMPTY], m, &numMoves);
  getRankMoves(ROOK, true, b->bb[BB_WR], b->bb[BB_BALL], b->bb[BB_EMPTY], m, &numMoves);
  getFileMoves(ROOK, true, b->bb[BB_WR], b->bb[BB_BALL], b->bb[BB_EMPTY], m, &numMoves);
  getRankMoves(QUEEN, true, b->bb[BB_WQ], b->bb[BB_BALL], b->bb[BB_EMPTY], m, &numMoves);
  getFileMoves(QUEEN, true, b->bb[BB_WQ], b->bb[BB_BALL], b->bb[BB_EMPTY], m, &numMoves);
  getDiagonalMoves(QUEEN, diagonal, true, b->bb[BB_WQ], b->bb[BB_BALL], b->bb[BB_EMPTY], m, &numMoves);
  getDiagonalMoves(QUEEN, antidiagonal, true, b->bb[BB_WQ], b->bb[BB_BALL], b->bb[BB_EMPTY], m, &numMoves);
  getKingMoves(b->bb[BB_WK], b->bb[BB_BALL], m, &numMoves);
  return numMoves;
}

/* Get all legal non-capturing moves on the given board if Black is to move */
int getBlackNonCaptures(Board *b, Move *m) {
  int numMoves = 0, fromSq, toSq;
  u64 mask;

  // Pawn pushes (one-step)
  mask = (b->bb[BB_BP] >> 8) & b->bb[BB_EMPTY];
  u64 oneStep = mask;
  while (mask) {
    GET_BIT_AND_CLEAR(mask, toSq);
    fromSq = toSq + 8;
    if (toSq >= 8) {
      pushMove(m, &numMoves, PAWN, fromSq, toSq, 0);
    } else {
      for (byte prom = KNIGHT; prom <= KING; prom++) {
        pushMove(m, &numMoves, PAWN, fromSq, toSq, prom);
      }
    }
  }

  // Pawn pushes (two-step)
  mask = ((oneStep & RANK_6) >> 8) & b->bb[BB_EMPTY];
  while (mask) {
    GET_BIT_AND_CLEAR(mask, toSq);
    pushMove(m, &numMoves, PAWN, toSq + 16, toSq, 0);
  }

  getKnightMoves(b->bb[BB_BN], b->bb[BB_EMPTY], m, &numMoves);
  getDiagonalMoves(BISHOP, diagonal, false, b->bb[BB_BB], 0, b->bb[BB_EMPTY], m, &numMoves);
  getDiagonalMoves(BISHOP, antidiagonal, false, b->bb[BB_BB], 0, b->bb[BB_EMPTY], m, &numMoves);
  getRankMoves(ROOK, false, b->bb[BB_BR], 0, b->bb[BB_EMPTY], m, &numMoves);
  getFileMoves(ROOK, false, b->bb[BB_BR], 0, b->bb[BB_EMPTY], m, &numMoves);
  getRankMoves(QUEEN, false, b->bb[BB_BQ], 0, b->bb[BB_EMPTY], m, &numMoves);
  getFileMoves(QUEEN, false, b->bb[BB_BQ], 0, b->bb[BB_EMPTY], m, &numMoves);
  getDiagonalMoves(QUEEN, diagonal, false, b->bb[BB_BQ], 0, b->bb[BB_EMPTY], m, &numMoves);
  getDiagonalMoves(QUEEN, antidiagonal, false, b->bb[BB_BQ], 0, b->bb[BB_EMPTY], m, &numMoves);
  getKingMoves(b->bb[BB_BK], b->bb[BB_EMPTY], m, &numMoves);
  return numMoves;
}

/* Get all legal moves on the given board if Black is to move */
int getBlackCaptures(Board* b, Move* m) {
  int numMoves = 0;
  getBlackPawnCaptures(b, FILE_H, 7, m, &numMoves);
  getBlackPawnCaptures(b, FILE_A, 9, m, &numMoves);
  getKnightMoves(b->bb[BB_BN], b->bb[BB_WALL], m, &numMoves);
  getDiagonalMoves(BISHOP, diagonal, true, b->bb[BB_BB], b->bb[BB_WALL], b->bb[BB_EMPTY], m, &numMoves);
  getDiagonalMoves(BISHOP, antidiagonal, true, b->bb[BB_BB], b->bb[BB_WALL], b->bb[BB_EMPTY], m, &numMoves);
  getRankMoves(ROOK, true, b->bb[BB_BR], b->bb[BB_WALL], b->bb[BB_EMPTY], m, &numMoves);
  getFileMoves(ROOK, true, b->bb[BB_BR], b->bb[BB_WALL], b->bb[BB_EMPTY], m, &numMoves);
  getRankMoves(QUEEN, true, b->bb[BB_BQ], b->bb[BB_WALL], b->bb[BB_EMPTY], m, &numMoves);
  getFileMoves(QUEEN, true, b->bb[BB_BQ], b->bb[BB_WALL], b->bb[BB_EMPTY], m, &numMoves);
  getDiagonalMoves(QUEEN, diagonal, true, b->bb[BB_BQ], b->bb[BB_WALL], b->bb[BB_EMPTY], m, &numMoves);
  getDiagonalMoves(QUEEN, antidiagonal, true, b->bb[BB_BQ], b->bb[BB_WALL], b->bb[BB_EMPTY], m, &numMoves);
  getKingMoves(b->bb[BB_BK], b->bb[BB_WALL], m, &numMoves);
  return numMoves;
}

int getAllMoves(Board* b, Move* m) {
  int numMoves = (b->side == WHITE) ? getWhiteCaptures(b, m) : getBlackCaptures(b, m);
  if (!numMoves) {
    numMoves = (b->side == WHITE) ? getWhiteNonCaptures(b, m) : getBlackNonCaptures(b, m);
  }
  return numMoves;
}
