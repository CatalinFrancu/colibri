#include <assert.h>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <string.h>
#include "board.h"
#include "egtb.h"
#include "precomp.h"

void printBoard(Board *b) {
  for (int row = 7; row >= 0; row--) {
    printf("  +---+---+---+---+---+---+---+---+\n");
    printf("%d |", row + 1);
    for (int col = 0; col < 8; col++) {
      char pieceName = ' ';
      u64 mask = 1ull << (8 * row + col);
      if ((b->bb[BB_WALL] ^ b->bb[BB_BALL]) & mask) {
        int p = PAWN;
        while (!((b->bb[BB_WALL + p] ^ b->bb[BB_BALL + p]) & mask)) {
          p++;
        }
        pieceName = PIECE_INITIALS[p];
        if (b->bb[BB_BALL] & mask) {
          pieceName += 'a' - 'A';
        }
      }
      printf(" %c |", pieceName);
    }
    printf("\n");
  }
  printf("  +---+---+---+---+---+---+---+---+\n   ");
  for (int col = 0; col < 8; col++) {
    printf(" %c  ", col + 'a');
  }
  int epSq = ctz(b->bb[BB_EP]);
  string epSqName = b->bb[BB_EP] ? SQUARE_NAME(epSq) : "none";
  printf("\nTo move: %s          En passant square: %s\n\n", (b->side == WHITE) ? "White" : "Black", epSqName.c_str());
}

void emptyBoard(Board *b) {
  memset(b, 0, sizeof(Board));
  b->bb[BB_EMPTY] = 0xffffffffffffffffull;
}

int getPieceCount(Board *b) {
  return popCount(b->bb[BB_WALL] ^ b->bb[BB_BALL]);
}

void rotateBoard(Board *b, int orientation) {
  for (int i = 0; i < BB_COUNT; i++) {
    b->bb[i] = rotate(b->bb[i], orientation);
  }
}

void changeSides(Board *b) {
  rotateBoard(b, ORI_FLIP_NS);
  for (int p = 0; p <= KING; p++) {
    u64 tmp = b->bb[BB_WALL + p];
    b->bb[BB_WALL + p] = b->bb[BB_BALL + p];
    b->bb[BB_BALL + p] = tmp;
  }
  b->side = WHITE + BLACK - b->side;
}

bool epCapturePossible(Board *b) {
  if (!b->bb[BB_EP]) {
    return false;
  }
  u64 captor;
  if (b->side == WHITE) {
    captor = b->bb[BB_WP] & RANK_5 & ((b->bb[BB_EP] >> 9) ^ (b->bb[BB_EP] >> 7));
  } else {
    captor = b->bb[BB_BP] & RANK_4 & ((b->bb[BB_EP] << 9) ^ (b->bb[BB_EP] << 7));
  }
  return (captor != 0ull);
}

void canonicalizeBoard(PieceSet *ps, int nps, Board *b) {
  // Boards where en passant capture is possible are first indexed by the EP square, which must be on the left-hand side.
  if (epCapturePossible(b)) {
    if (b->bb[BB_EP] & 0xf0f0f0f0f0f0f0f0ull) {
      rotateBoard(b, ORI_FLIP_EW);
    }
    return;
  } else {
    b->bb[BB_EP] = 0ull;
  }
  int base = (ps[0].side == WHITE) ? BB_WALL : BB_BALL;
  u64 mask = b->bb[base + ps[0].piece];
  if (ps[0].piece == PAWN) {
    mask >>= 8;
  }
  int comb = rankCombination(mask, 0);
  int canonical;
  if (ps[0].piece == PAWN) {
    canonical = canonical48[ps[0].count][comb];
  } else {
    canonical = canonical64[ps[0].count][comb];
  }
  if (canonical < 0) {
    rotateBoard(b, -canonical);
  }
}

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

string boardToFen(Board *b) {
  stringstream result;
  for (int i = 56; i >= 0; i -= 8) {
    int numEmpty = 0;
    for (int sq = i; sq < i + 8; sq++) {
      u64 mask = 1ull << sq;
      if (b->bb[BB_EMPTY] & mask) {
        numEmpty++;
      } else {
        if (numEmpty) {
          result << numEmpty;
        }
        numEmpty = 0;
        int p = PAWN;
        while (!((b->bb[BB_WALL + p] ^ b->bb[BB_BALL + p]) & mask)) {
          p++;
        }
        char pieceName = PIECE_INITIALS[p];
        if (b->bb[BB_BALL] & mask) {
          pieceName += 'a' - 'A';
        }
        result << pieceName;
      }
    }
    if (numEmpty) {
      result << numEmpty;
    }
    if (i) {
      result << "/";
    }
  }
  result << " " << ((b->side == WHITE) ? 'w' : 'b') << " - "; // No castling
  if (b->bb[BB_EP]) {
    int epSq = ctz(b->bb[BB_EP]);
    result << SQUARE_NAME(epSq);
  } else {
    result << "-";
  }
  result << " 0 0"; // Nothing for the halfmove clock / move number
  return result.str();
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
    u64 capturedPawn = (b->side == WHITE) ? (toMask >> 8) : (toMask << 8);
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

void makeBackwardMove(Board *b, Move m) {
  int base = (b->side == WHITE) ? BB_BALL : BB_WALL;
  u64 mask = (1ull << m.from) ^ (1ull << m.to);
  b->bb[base] ^= mask;
  b->bb[base + m.piece] ^= mask;
  b->bb[BB_EMPTY] ^= mask;
  b->bb[BB_EP] = 0ull;
  b->side = WHITE + BLACK - b->side;
}


int evalBoard(Board *b) {
  if (!b->bb[BB_WALL]) {
    return (b->side == WHITE) ? 1 : -1; // Won/lost now
  }
  if (!b->bb[BB_BALL]) {
    return (b->side == WHITE) ? -1 : 1; // Lost/won now
  }
  return egtbLookup(b);
}
