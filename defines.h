#ifndef __DEFINES_H__
#define __DEFINES_H__
using namespace std;

/* Sides */
#define WHITE false
#define BLACK true

/* Piece types */
#define PAWN 1
#define KNIGHT 2
#define BISHOP 3
#define ROOK 4
#define QUEEN 5
#define KING 6

/* Order of bitboards in a board */
/* The intervals BB_WALL - BB_WK and BB_BALL - BB_BK need to stay contiguous because makeMove depends on it. */
#define BB_WALL 0
#define BB_WP 1
#define BB_WN 2
#define BB_WB 3
#define BB_WR 4
#define BB_WQ 5
#define BB_WK 6
#define BB_BALL 7
#define BB_BP 8
#define BB_BN 9
#define BB_BB 10
#define BB_BR 11
#define BB_BQ 12
#define BB_BK 13
#define BB_EMPTY 14
#define BB_EP 15
#define BB_COUNT 16

/* Possible orientations */
#define ORI_NORMAL 0          /* c1 -> c1 */
#define ORI_ROT_CCW 1         /* c1 -> h3 */
#define ORI_ROT_180 2         /* c1 -> f8 */
#define ORI_ROT_CW 3          /* c1 -> a6 */
#define ORI_FLIP_NS 4         /* c1 -> c8 */
#define ORI_FLIP_DIAG 5       /* c1 -> a3 */
#define ORI_FLIP_EW 6         /* c1 -> f1 */
#define ORI_FLIP_ANTIDIAG 7   /* c1 -> h6 */

/* Masks for some ranks and files of interest */
#define FILE_A 0x0101010101010101ull
#define FILE_H 0x8080808080808080ull
#define RANK_1 0x00000000000000ffull
#define RANK_3 0x0000000000ff0000ull
#define RANK_6 0x0000ff0000000000ull
#define RANK_8 0xff00000000000000ull

/* For assorted purposes */
#define INFTY 1000000000

/* If a bitboard is mutiplied by FILE_MAGIC, all the bits on the 'a' file will end up on the 8-th rank. */
#define FILE_MAGIC 0x8040201008040201ull

/* If a diagonal is mutiplied by DIAG_MAGIC, all the bits on the diagonal will end up on the 8-th rank. */
#define DIAG_MAGIC 0x0101010101010101ull

/* The board at the start of a new game */
#define NEW_BOARD "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"

/* Clears the rightmost set bit of x and returns its index in square */
#define GET_BIT_AND_CLEAR(x, square)            \
  square = ctz(x);      \
  x &= x - 1;

/* Maximum number of pieces in the endgame tables */
#define EGTB_MEN 5

/* EGTB scores are shifted by 1, e.g. -1 means "lost now", +1 means "won now", -7 means "losing in 6 half-moves */
#define EGTB_DRAW 0

/* Directions of move generation. BACKWARD is used for retrograde analysis during EGTB generation */
#define FORWARD true
#define BACKWARD false

/* TODO: add a config file */
#define EGTB_PATH "/home/cata/public_html/colibri/egtb"
#define QUERY_SERVER_PORT 2359

/* The square between 0 and 63 corresponding to a rank and file between 0 and 7 */
#define SQUARE(rank, file) (((rank) << 3) + (file))

/* The 64-bit mask corresponding to a rank and file between 0 and 7 */
#define BIT(rank, file) (1ull << (((rank) << 3) + (file)))

/* Return a square given its algebraic coordinates, e.g. "a1" -> 0, "h8" -> 63 */
#define SQUARE_BY_NAME(s) ((((s).at(1) - '1') << 3) + ((s).at(0) - 'a'))

/* Return a 64-bit mask with one bit set given its algebraic coordinates e.g. "h8" -> 0x8000000000000000 */
#define BIT_BY_NAME(s) (1ull << SQUARE_BY_NAME((s)))

/* Return a 1-character string naming the file (column) of the given square */
#define FILE_NAME(sq) (string(1, 'a' + (sq & 7)))

/* Return a 1-character string naming the rank (row) of the given square */
#define RANK_NAME(sq) (string(1, '1' + (sq >> 3)))

/* Return a 2-character string naming the given square */
#define SQUARE_NAME(sq) (FILE_NAME(sq) + RANK_NAME(sq))

typedef unsigned long long u64;
typedef unsigned char byte;

typedef struct {
  u64 bb[BB_COUNT]; /* One bitboard for every BB_* property above */
  bool side;        /* Side to move */
} Board;

/* There is some redundancy in our Move structure. The aim is not to save space, since we don't store moves during our algorithms.
 * The aim is to print moves easily. */
typedef struct Mmove {
  byte piece;     /* Type of piece being moved */
  byte from;      /* Square of origin */
  byte to;        /* Destination square */
  byte promotion; /* Promotion, if any */
} Move;

/* For EGTB, use PieceSet's in the order in which they are placed on the board */
typedef struct PieceSet {
  bool side;
  byte piece;
  byte count;
} PieceSet;

/* Piece names for move notation */
const char PIECE_INITIALS[8] = " PNBRQK";

#endif
