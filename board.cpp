#include <assert.h>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <string.h>
#include "board.h"
#include "egtb.h"
#include "logging.h"
#include "movegen.h"
#include "precomp.h"

void printBoard(Board *b) {
  if (!b) {
    printf("null board pointer\n");
    return;
  }
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

string getMoveName(Move m) {
  string s(PIECE_INITIALS[m.piece] + SQUARE_NAME(m.from) + SQUARE_NAME(m.to));
  if (m.promotion) {
    s += '=' + PIECE_INITIALS[m.promotion];
  }
  return s;
}

void emptyBoard(Board *b) {
  memset(b, 0, sizeof(Board));
  b->bb[BB_EMPTY] = 0xffffffffffffffffull;
}

bool equalBoard(Board *b1, Board *b2) {
  for (int i = 0; i < BB_COUNT; i++) {
    if (b1->bb[i] != b2->bb[i]) {
      return false;
    }
  }
  return (b1->side == b2->side);
}

bool equalMove(Move m1, Move m2) {
  return m1.piece == m2.piece && m1.from == m2.from && m1.to == m2.to && m1.promotion == m2.promotion;
}

int getPieceCount(Board *b) {
  return popCount(b->bb[BB_WALL] ^ b->bb[BB_BALL]);
}

void rotateBoard(Board *b, int orientation) {
  switch (orientation) {
    case ORI_NORMAL:
      return;
    case ORI_FLIP_EW:
      for (int i = 0; i < BB_COUNT; i++) {
        b->bb[i] = mirrorEW(b->bb[i]);
      }
      return;
    case ORI_ROT_CCW:
      for (int i = 0; i < BB_COUNT; i++) {
        b->bb[i] = flipDiagA1H8(reverseBytes(b->bb[i]));
      }
      return;
    case ORI_ROT_180:
      for (int i = 0; i < BB_COUNT; i++) {
        b->bb[i] = reverseBytes(mirrorEW(b->bb[i]));
      }
      return;
    case ORI_ROT_CW:
      for (int i = 0; i < BB_COUNT; i++) {
        b->bb[i] = reverseBytes(flipDiagA1H8(b->bb[i]));
      }
      return;
    case ORI_FLIP_NS:
      for (int i = 0; i < BB_COUNT; i++) {
        b->bb[i] = reverseBytes(b->bb[i]);
      }
      return;
    case ORI_FLIP_DIAG:
      for (int i = 0; i < BB_COUNT; i++) {
        b->bb[i] = flipDiagA1H8(b->bb[i]);
      }
      return;
    case ORI_FLIP_ANTIDIAG:
      for (int i = 0; i < BB_COUNT; i++) {
        b->bb[i] = flipDiagA8H1(b->bb[i]);
      }
      return;
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

void changeSidesIfNeeded(Board *b, int wp, int bp) {
  bool change = false;
  if (wp < bp) {
    change = true;
  } else if (wp == bp) {
    int p = KING + 1, whiteGroup, blackGroup;
    do {
      p--;
      whiteGroup = popCount(b->bb[BB_WALL + p]);
      blackGroup = popCount(b->bb[BB_BALL + p]);
    } while ((p > PAWN) && (whiteGroup == blackGroup));
    change = (whiteGroup < blackGroup);
  }
  if (change) {
    changeSides(b);
  }
}

bool isCapture(Board *b, Move m) {
  u64 toMask = 1ull << m.to;
  return ((m.piece == PAWN) && (toMask == b->bb[BB_EP])) || (toMask & ~b->bb[BB_EMPTY]);
}

inline u64 getMaskFromPieceSet(Board* b, PieceSet ps) {
  int base = (ps.side == WHITE) ? BB_WALL : BB_BALL;
  u64 mask = b->bb[base + ps.piece];
  if (ps.piece == PAWN) {
    mask >>= 8;
  }
  return mask;
}

int canonicalizeBoard(PieceSet *ps, int nps, Board *b, bool dryRun) {
  // Boards where en passant capture is possible are first indexed by the EP
  // square, which must be on the left-hand side.
  if (epCapturePossible(b)) {
    if (b->bb[BB_EP] & 0xf0f0f0f0f0f0f0f0ull) {
      if (!dryRun) {
        rotateBoard(b, ORI_FLIP_EW);
      }
      return ORI_FLIP_EW;
    }
    return ORI_NORMAL;
  } else {
    if (!dryRun) {
      b->bb[BB_EP] = 0ull;
    }
  }

  // Start with the rotation mask for the first piece set.
  u64 mask = getMaskFromPieceSet(b, ps[0]);
  int comb = rankCombinationFree(mask);

  byte trMask = (ps[0].piece == PAWN)
    ? rotMask48[ps[0].count][comb]
    : rotMask64[ps[0].count][comb];
  int i = 1, tr;

  // Process more piece sets
  while ((i < nps) && (popCount(trMask) > 1)) {
    mask = getMaskFromPieceSet(b, ps[i]);

    // examine all transforms and save all those generating the smallest combo
    byte newTrMask = 0;
    int minComb = INFTY;
    while (trMask) {
      GET_BIT_AND_CLEAR(trMask, tr);
      comb = rankCombinationFree(rotate(mask, tr));
      if (comb < minComb) {           // new minimum found, start a new mask
        minComb = comb;
        newTrMask = 1 << tr;
      } else if (comb == minComb) {   // add to the existing transform mask
        newTrMask |= 1 << tr;
      }
    }
    trMask = newTrMask;
    i++;
  }

  GET_BIT_AND_CLEAR(trMask, tr); // take the last set bit
  if (!dryRun) {
    rotateBoard(b, tr);
  }
  return tr;
}

Board* fenToBoard(const char *fen) {
  // Use a static board, then allocate it on the heap and copy it if all goes well
  // This way we can return NULL on errors and not worry about deallocation.
  Board b;
  memset(&b, 0, sizeof(Board));

  // Expect 64 squares' worth of pieces and empty spaces, with slashes every 8 squares
  int rank = 7, file = 0, bitboard;
  const char *s = fen;
  while (rank > 0 || file != 8) {
    switch (*s) {
      case 'K': case 'Q': case 'R': case 'B': case 'N': case 'P':
      case 'k': case 'q': case 'r': case 'b': case 'n': case 'p':
        if (file == 8) {
          return NULL;
        }
        bitboard = isupper(*s) ? (BB_WALL + PIECE_BY_NAME[*s - 'A']) : (BB_BALL + PIECE_BY_NAME[*s - 'a']);
        b.bb[bitboard] |= 1ull << (8 * rank + file);
        file++;
        break;
      case '1': case '2': case '3': case '4':
      case '5': case '6': case '7': case '8':
        file += *s - '0';
        if (file > 8) {
          return NULL;
        }
        break;
      case '/':
        if (file != 8 || rank == 0) {
          return NULL;
        }
        rank--;
        file = 0;
        break;
      default: return NULL;
    }
    s++;
  }

  // Consume a space
  if (*s != ' ') {
    return NULL;
  }
  s++;

  // Consume the stm
  if (*s == 'w') {
    b.side = WHITE;
  } else if (*s == 'b') {
    b.side = BLACK;
  } else {
    return NULL;
  }
  s++;

  // Consume a space, castling ability (-) and a space
  if (*s != ' ' || *(s + 1) != '-' || *(s + 2) != ' ') {
    return NULL;
  }
  s += 3;

  // Consume the en passant info (- or square)
  int epSquare;
  if (*s == '-') {
    b.bb[BB_EP] = 0ull;
    s++;
  } else if (*s >= 'a' && *s <= 'h') {
    file = *s - 'a';
    s++;
    if (*s < '1' || *s > '8') {
      return NULL;
    }
    rank = *s - '1';
    epSquare = SQUARE(rank, file);
    b.bb[BB_EP] = 1ull << epSquare;
    s++;
  } else {
    return NULL;
  }

  // Consume a space
  if (*s != ' ') {
    return NULL;
  }
  s++;

  // Ignore the rest of the string (halfmove clock, space, fullmove number)

  // Construct the remaining fields
  b.bb[BB_WALL] = b.bb[BB_WK] | b.bb[BB_WQ] | b.bb[BB_WR] | b.bb[BB_WB] | b.bb[BB_WN] | b.bb[BB_WP];
  b.bb[BB_BALL] = b.bb[BB_BK] | b.bb[BB_BQ] | b.bb[BB_BR] | b.bb[BB_BB] | b.bb[BB_BN] | b.bb[BB_BP];
  b.bb[BB_EMPTY] = ~(b.bb[BB_WALL] ^ b.bb[BB_BALL]);

  // Integrity check: pawns on the 1st or 8th rank
  if ((b.bb[BB_WP] ^ b.bb[BB_BP]) & (RANK_1 ^ RANK_8)) {
    return NULL;
  }

  if (b.bb[BB_EP]) {
    u64 epRank = (b.side == WHITE) ? RANK_6 : RANK_3;
    u64 behindEp = (b.side == WHITE) ? (b.bb[BB_EP] << 8) : (b.bb[BB_EP] >> 8);
    u64 inFrontOfEp = (b.side == WHITE) ? (b.bb[BB_EP] >> 8) : (b.bb[BB_EP] << 8);
    u64 pawnSet = (b.side == WHITE) ? b.bb[BB_BP] : b.bb[BB_WP]; // Piece that should be in front of the en passant square
    // Hee hee, I said pawn set

    // Integrity check: en passant square must be empty
    if (b.bb[BB_EP] & ~b.bb[BB_EMPTY]) {
      return NULL;
    }
    // Integrity check: en passant square must be on the 3rd or 6th rank
    if (!(b.bb[BB_EP] & epRank)) {
      return NULL;
    }
    // Integrity check: square behind ep square must be empty
    if (!(behindEp & b.bb[BB_EMPTY])) {
      return NULL;
    }
    // Integrity check: square in front of ep square must be a pawn
    if (!(inFrontOfEp & pawnSet)) {
      return NULL;
    }
  }

  Board *result = (Board*)malloc(sizeof(Board));
  memcpy(result, &b, sizeof(Board));
  return result;
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

void makeMoveForSide(Board *b, Move m, int allMe, int allYou) {
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
  b->side = 1 - b->side;
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
  b->side = 1 - b->side;
}

Board* makeMoveSequence(int numMoveStrings, string *moveStrings) {
  Board *b = fenToBoard(NEW_BOARD);
  Move m[MAX_MOVES];
  string san[MAX_MOVES];
  int numLegalMoves;
  int i = 0;
  while ((i < numMoveStrings) && b) {
    numLegalMoves = getAllMoves(b, m, FORWARD);
    // Get the SAN for every legal move in this position
    getAlgebraicNotation(b, m, numLegalMoves, san);
    int j = 0;
    while ((j < numLegalMoves) && (san[j] != moveStrings[i])) {
      j++;
    }
    if (j < numLegalMoves) {
      makeMove(b, m[j]);
    } else {
      die("Bad move: %s", moveStrings[i].c_str());
    }
    i++;
  }
  return b;
}
