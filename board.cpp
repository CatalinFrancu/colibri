#include <assert.h>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <string.h>
#include "board.h"
#include "precomp.h"
using namespace std;

Board fenToBoard(const char *fen) {
  istringstream ins;
  ins.str(fen);
  Board b;
  memset(&b, 0, sizeof(b));

  string pieces;
  ins >> pieces;

  int square = 56;
  for (string::iterator it = pieces.begin(); it != pieces.end(); it++ ) {
    char c = *it;
    if (c == 'K') {
      b.bb[BB_WK] |= 1ull << square;
    } else if (c == 'Q') {
      b.bb[BB_WQ] |= 1ull << square;
    } else if (c == 'R') {
      b.bb[BB_WR] |= 1ull << square;
    } else if (c == 'B') {
      b.bb[BB_WB] |= 1ull << square;
    } else if (c == 'N') {
      b.bb[BB_WN] |= 1ull << square;
    } else if (c == 'P') {
      b.bb[BB_WP] |= 1ull << square;
    } else if (c == 'k') {
      b.bb[BB_BK] |= 1ull << square;
    } else if (c == 'q') {
      b.bb[BB_BQ] |= 1ull << square;
    } else if (c == 'r') {
      b.bb[BB_BR] |= 1ull << square;
    } else if (c == 'b') {
      b.bb[BB_BB] |= 1ull << square;
    } else if (c == 'n') {
      b.bb[BB_BN] |= 1ull << square;
    } else if (c == 'p') {
      b.bb[BB_BP] |= 1ull << square;
    }

    if (isdigit(c)) {
      square += (c - '0');
    } else if (isalpha(c)) {
      square++;
    }

    if (c == '/') {
      assert(square % 8 == 0);
      square -= 16;
    }
  }
  assert(square == 8);
  b.bb[BB_WALL] = b.bb[BB_WK] | b.bb[BB_WQ] | b.bb[BB_WR] | b.bb[BB_WB] | b.bb[BB_WN] | b.bb[BB_WP];
  b.bb[BB_BALL] = b.bb[BB_BK] | b.bb[BB_BQ] | b.bb[BB_BR] | b.bb[BB_BB] | b.bb[BB_BN] | b.bb[BB_BP];
  b.bb[BB_EMPTY] = 0xffffffffffffffffull ^ b.bb[BB_WALL] ^ b.bb[BB_BALL];

  string s;
  ins >> s;
  b.side = (s == "w") ? WHITE : BLACK;

  ins >> s; // ignore castling ability
  ins >> s;
  b.bb[BB_EP] = (s == "-") ? 0ull : BIT_BY_NAME(s);

  // ins >> b.halfMove; // Ignore this
  return b;
}

void getAlgebraicNotation(Board *b, Move *m, int numMoves, string *san) {
  for (int i = 0; i < numMoves; i++) {
    if (m[i].piece == PAWN) {
      if ((m[i].from & 7) == (m[i].to & 7)) {
        san[i] = SQUARE_NAME(m[i].to);
      } else {
        san[i] = FILE_NAME(m[i].from) + "x" + SQUARE_NAME(m[i].to);
      }
      if (m[i].promotion) {
        san[i] += "=" + string(1, PIECE_INITIALS[m[i].promotion]);
      }
    } else {
      san[i] = PIECE_INITIALS[m[i].piece];
      // Count the number of identical pieces that start in the same row / column and can go to the same square.
      int sameRank = 0, sameFile = 0, diffRankAndFile = 0;
      for (int j = 0; j < numMoves; j++) {
        if ((j != i) && (m[j].piece == m[i].piece) && (m[j].to == m[i].to)) {
          if ((m[j].from >> 3) == (m[i].from >> 3)) {
            sameRank++;
          } else if ((m[j].from & 7) == (m[i].from & 7)) {
            sameFile++;
          } else {
            diffRankAndFile++;
          }
        }
      }

      // Qualify the starting square as Nbd2, N1d2 or Nb1d2 if needed
      if (sameRank && sameFile) {
        san[i] += SQUARE_NAME(m[i].from);
      } else if (sameFile) {
        san[i] += RANK_NAME(m[i].from);
      } else if (sameRank || diffRankAndFile) {
        san[i] += FILE_NAME(m[i].from);
      }

      // Add an x for the capture
      if ((1ull << m[i].to) & ~b->bb[BB_EMPTY]) {
        san[i] += "x";
      }

      san[i] += SQUARE_NAME(m[i].to);
    }
  }
}

void makeMoveForSide(Board* b, Move m, int allMe, int allYou) {
  u64 fromMask = 1ull << m.from;
  u64 toMask = 1ull << m.to;

  // If an en passant capture, remove the captured pawn
  if ((m.piece == PAWN) && (toMask == b->bb[BB_EP])) {
    u64 capturedPawn = 1ull << ((m.from & ~7) + (m.to & 7));
    b->bb[allYou + PAWN] ^= capturedPawn;
    b->bb[allYou] ^= capturedPawn;
  }

  // Set the enPassant bit
  if ((m.piece == PAWN) && abs(m.from - m.to) == 16) {
    b->bb[BB_EP] = 1ull << ((m.from + m.to) >> 1);
  } else {
    b->bb[BB_EP] = 0;
  }

  // Set the side-to-move's pieces
  b->bb[allMe] ^= fromMask ^ toMask;
  b->bb[allMe + m.piece] ^= fromMask;
  if (m.promotion) {
    b->bb[allMe + m.promotion] ^= toMask;
  } else {
    b->bb[allMe + m.piece] ^= toMask;
  }

  // Clear the side-not-to-move piece if necessary
  if (b->bb[allYou] & toMask) {
    for (int i = allYou; i <= allYou + KING; i++) {
      b->bb[i] &= ~toMask;
    }
  }

  b->bb[BB_EMPTY] = ~(b->bb[BB_WALL] ^ b->bb[BB_BALL]);
  b->side = WHITE + BLACK - b->side;
}

void makeMove(Board* b, Move m) {
  if (b->side == WHITE) {
    makeMoveForSide(b, m, BB_WALL, BB_BALL);
  } else {
    makeMoveForSide(b, m, BB_BALL, BB_WALL);
  }
}
