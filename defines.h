#ifndef __DEFINES_H__
#define __DEFINES_H__
#include <stddef.h>
#include <unordered_map>
#include <unordered_set>
using namespace std;

/* Config file name, without the directory. The file is assumed to reside in the colibri/ installation directory. */
#define CONFIG_FILE "colibri.conf"

/* Sides */
#define WHITE 0
#define BLACK 1

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

const int ORIENTATIONS[8] =
  { ORI_NORMAL, ORI_FLIP_EW, ORI_ROT_CCW, ORI_ROT_180,
    ORI_ROT_CW, ORI_FLIP_NS, ORI_FLIP_DIAG, ORI_FLIP_ANTIDIAG };

/* Orientation we need to apply to undo an orientation */
const int REVERSE_ORIENTATION[8] =
  { ORI_NORMAL, ORI_ROT_CW, ORI_ROT_180, ORI_ROT_CCW,
    ORI_FLIP_NS, ORI_FLIP_DIAG, ORI_FLIP_EW, ORI_FLIP_ANTIDIAG };

/* Masks for some ranks and files of interest */
#define FILE_A 0x0101010101010101ull
#define FILE_B 0x0202020202020202ull
#define FILE_H 0x8080808080808080ull
#define RANK_1 0x00000000000000ffull
#define RANK_3 0x0000000000ff0000ull
#define RANK_4 0x00000000ff000000ull
#define RANK_5 0x000000ff00000000ull
#define RANK_6 0x0000ff0000000000ull
#define RANK_8 0xff00000000000000ull

/* For assorted purposes */
#define INFTY 1000000000
#define INFTY64 1000000000000000000ull

/* NULL-like value for statically allocated lists */
#define NIL 1000000000

/* If a bitboard is mutiplied by FILE_MAGIC, all the bits on the 'a' file will end up on the 8-th rank. */
#define FILE_MAGIC 0x8040201008040201ull

/* If a diagonal is mutiplied by DIAG_MAGIC, all the bits on the diagonal will end up on the 8-th rank. */
#define DIAG_MAGIC 0x0101010101010101ull

/* The board at the start of a new game */
#define NEW_BOARD "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w - - 0 1"

/* Clears the rightmost set bit of x and returns its index in square */
#define GET_BIT_AND_CLEAR(x, square) \
  square = ctz(x);                   \
  x &= x - 1;

/* Maximum number of legal moves in a position */
#define MAX_MOVES 200

/* Maximum number of pieces in the endgame tables */
#define EGTB_MEN 5

#define EGTB_UNKNOWN 1000000000

/* Directions of move generation. BACKWARD is used for retrograde analysis during EGTB generation */
#define FORWARD true
#define BACKWARD false

/* Cache EGTB files in 32 KB chunks */
#define EGTB_CHUNK_SIZE 32768

/* Size of every PNS^1 step */
#define PNS_STEP_SIZE 1000000

/* Size of the in-memory PN^2 book. */
#define PNS_BOOK_SIZE 10000000

/* Size of the PN^2 move array */
#define PNS_MOVE_SIZE 10000000

/* Size of the PN^2 child array */
#define PNS_CHILD_SIZE 10000000

/* Size of the PN^2 parent array */
#define PNS_PARENT_SIZE 10000000

/* The square between 0 and 63 corresponding to a rank and file between 0 and 7 */
#define SQUARE(rank, file) (((rank) << 3) + (file))

/* The 64-bit mask corresponding to a rank and file between 0 and 7 */
#define BIT(rank, file) (1ull << (((rank) << 3) + (file)))

/* Return a square given its algebraic coordinates, e.g. "a1" -> 0, "h8" -> 63 */
#define SQUARE_BY_NAME(s) ((((s).at(1) - '1') << 3) + ((s).at(0) - 'a'))

/* Return a 1-character string naming the file (column) of the given square */
#define FILE_NAME(sq) (string(1, 'a' + (sq & 7)))

/* Return a 1-character string naming the rank (row) of the given square */
#define RANK_NAME(sq) (string(1, '1' + (sq >> 3)))

/* Return a 2-character string naming the given square */
#define SQUARE_NAME(sq) (FILE_NAME(sq) + RANK_NAME(sq))

#define MIN(a,b) \
  ({ __typeof__ (a) _a = (a); \
     __typeof__ (b) _b = (b); \
     _a < _b ? _a : _b; })

/* Commands given from the command line */
#define CMD_ANALYZE 1
#define CMD_SERVER 2

typedef unsigned long long u64;
typedef unsigned char byte;

typedef struct {
  u64 bb[BB_COUNT]; /* One bitboard for every BB_* property above */
  bool side;        /* Side to move */
} Board;

/* There is some redundancy in our Move structure. The aim is not to save space, since we don't store moves during our algorithms.
 * The aim is to print moves easily. */
typedef struct {
  byte piece;     /* Type of piece being moved */
  byte from;      /* Square of origin */
  byte to;        /* Destination square */
  byte promotion; /* Promotion, if any */
} Move;

/* For EGTB, use PieceSet's in the order in which they are placed on the board */
typedef struct {
  bool side;
  byte piece;
  byte count;
} PieceSet;

/* Piece names for move notation */
const char PIECE_INITIALS[8] = " PNBRQK";
const int PIECE_BY_NAME[26] = { 0, BISHOP, 0, 0, 0, 0, 0, 0, 0, 0, KING, 0, 0, KNIGHT, 0, PAWN, QUEEN, ROOK, 0, 0, 0, 0, 0, 0, 0, 0 };

/* A PN search tree node (it's really a DAG). Pointers are statically represented in a preallocated memory area. */
typedef struct {
  u64 proof;
  u64 disproof;
  int child;     // index in pnsChildren of the first child
  int move;      // index in pnsMove of the first move
  int parent;    // index in pnsParent of the first parent (transpositions have multiple parents in the DAG)
  byte numChildren;
} PnsNode;

/* A linked list of PnsNodes. Statically allocated. */
typedef struct {
  int node;
  int next;
} PnsNodeList;

inline int sgn(int x) {
  return (x > 0) - (x < 0);
}

#endif
