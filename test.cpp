#define BOOST_TEST_MODULE colibriTest
#include <boost/test/included/unit_test.hpp>
#include "bitmanip.h"
#include "movegen.h"
#include "precomp.h"

/************************* Tests for bitmanip.cpp *************************/

BOOST_AUTO_TEST_CASE(testReverseBytes) {
  BOOST_CHECK_EQUAL(reverseBytes(0x1011121314151617ull), 0x1716151413121110ull);
}

BOOST_AUTO_TEST_CASE(testPopCount) {
  BOOST_CHECK_EQUAL(popCount(0x0000000000000000ull), 0);
  BOOST_CHECK_EQUAL(popCount(0x0000000000000001ull), 1);
  BOOST_CHECK_EQUAL(popCount(0x0000010000000001ull), 2);
  BOOST_CHECK_EQUAL(popCount(0x0000010000800001ull), 3);
  BOOST_CHECK_EQUAL(popCount(0x0400010000008001ull), 4);
  BOOST_CHECK_EQUAL(popCount(0xffffffffffffffffull), 64);
}

BOOST_AUTO_TEST_CASE(testCtz) {
  BOOST_CHECK_EQUAL(ctz(0x0000000000000001ull), 0);
  BOOST_CHECK_EQUAL(ctz(0x0000000000000002ull), 1);
  BOOST_CHECK_EQUAL(ctz(0x0000000000000004ull), 2);
  BOOST_CHECK_EQUAL(ctz(0x000fc00000000004ull), 2);
  BOOST_CHECK_EQUAL(ctz(0x0000100000000000ull), 44);
  BOOST_CHECK_EQUAL(ctz(0x0200000000000000ull), 57);
  BOOST_CHECK_EQUAL(ctz(0x8000000000000000ull), 63);
}

BOOST_AUTO_TEST_CASE(testRotate) {
  BOOST_CHECK_EQUAL(rotate(0x0000000c0000d001ull, ORI_NORMAL), 0x0000000c0000d001ull);
  BOOST_CHECK_EQUAL(rotate(0x0000000000000001ull, ORI_ROT_CCW), 0x0000000000000080ull);
  BOOST_CHECK_EQUAL(rotate(0x8000000000000000ull, ORI_ROT_CCW), 0x0100000000000000ull);
  BOOST_CHECK_EQUAL(rotate(0x4000000400001001ull, ORI_ROT_CCW), 0x0001004000080080ull);
  BOOST_CHECK_EQUAL(rotate(0x4000000400001001ull, ORI_ROT_180), 0x8008000020000002ull);
  BOOST_CHECK_EQUAL(rotate(0x4000000400001001ull, ORI_ROT_CW), 0x0100100002008000ull);
  BOOST_CHECK_EQUAL(rotate(0x4000000400001001ull, ORI_FLIP_NS), 0x0110000004000040ull);
  BOOST_CHECK_EQUAL(rotate(0x4000000400001001ull, ORI_FLIP_DIAG), 0x0080000200100001ull);
  BOOST_CHECK_EQUAL(rotate(0x4000000400001001ull, ORI_FLIP_EW), 0x0200002000000880ull);
  BOOST_CHECK_EQUAL(rotate(0x4000000400001001ull, ORI_FLIP_ANTIDIAG), 0x8000080040000100ull);
}

BOOST_AUTO_TEST_CASE(tetGetBitAndClear) {
  u64 x = 0x0000806400000000ull;
  int sq;
  GET_BIT_AND_CLEAR(x, sq);
  BOOST_CHECK_EQUAL(x, 0x0000806000000000ull);
  BOOST_CHECK_EQUAL(sq, 34);
  GET_BIT_AND_CLEAR(x, sq);
  BOOST_CHECK_EQUAL(x, 0x0000804000000000ull);
  BOOST_CHECK_EQUAL(sq, 37);
  GET_BIT_AND_CLEAR(x, sq);
  BOOST_CHECK_EQUAL(x, 0x0000800000000000ull);
  BOOST_CHECK_EQUAL(sq, 38);
  GET_BIT_AND_CLEAR(x, sq);
  BOOST_CHECK_EQUAL(x, 0);
  BOOST_CHECK_EQUAL(sq, 47);
}

/************************* Tests for precomp.cpp *************************/

BOOST_AUTO_TEST_CASE(testNChooseK) {
  precomputeAll();
  BOOST_CHECK_EQUAL(choose[4][1], 4);
  if (EGTB_MEN > 3) {
    BOOST_CHECK_EQUAL(choose[6][2], 15);
    BOOST_CHECK_EQUAL(choose[8][4], 70);
    BOOST_CHECK_EQUAL(choose[64][3], 41664);
    BOOST_CHECK_EQUAL(choose[64][4], 635376);
  }
}

BOOST_AUTO_TEST_CASE(testRankCombination) {
  BOOST_CHECK_EQUAL(rankCombination(0x8000000000000000ull, 0), 63);
  if (EGTB_MEN > 3) {
    BOOST_CHECK_EQUAL(rankCombination(0x000000000000000full, 0), 0);
    BOOST_CHECK_EQUAL(rankCombination(0xf000000000000000ull, 0), 635375);
    BOOST_CHECK_EQUAL(rankCombination(0x0000010000400000ull, 0), 802);
    BOOST_CHECK_EQUAL(rankCombination(0x0000f00000000000ull, 0), 194579);
    BOOST_CHECK_EQUAL(rankCombination(0x0000000001000040ull, 0), 282);
    BOOST_CHECK_EQUAL(rankCombination(0x0000000000000007ull, 0), 0);

    // Now some test cases where some squares are already taken, as indicated by the third argument
    BOOST_CHECK_EQUAL(rankCombination(0x000000000000000full, 0x0000000010101080ull), 0);
    BOOST_CHECK_EQUAL(rankCombination(0x0000000000000c00ull, 0x000000000000000full), 27);
    BOOST_CHECK_EQUAL(rankCombination(0x0f00000000000000ull, 0xf0f0f0f0f0f0f0f0ull), 35959);
    BOOST_CHECK_EQUAL(rankCombination(0x24ull, 0x11ull), 4);
  }
}

BOOST_AUTO_TEST_CASE(testUnrankCombination) {
  BOOST_CHECK_EQUAL(unrankCombination(63, 1), 0x8000000000000000ull);
  if (EGTB_MEN > 3) {
    BOOST_CHECK_EQUAL(unrankCombination(0, 4), 0x000000000000000full);
    BOOST_CHECK_EQUAL(unrankCombination(635375, 4), 0xf000000000000000ull);
    BOOST_CHECK_EQUAL(unrankCombination(802, 2), 0x0000010000400000ull);

    BOOST_CHECK_EQUAL(unrankCombination(194579, 4), 0x0000f00000000000ull);
    BOOST_CHECK_EQUAL(unrankCombination(282, 2), 0x0000000001000040ull);
    BOOST_CHECK_EQUAL(unrankCombination(0, 3), 0x0000000000000007ull);
  }
}

BOOST_AUTO_TEST_CASE(testKingAttacks) {
  BOOST_CHECK_EQUAL(kingAttacks[0], 0x0000000000000302ull);
  BOOST_CHECK_EQUAL(kingAttacks[7], 0x000000000000c040ull);
  BOOST_CHECK_EQUAL(kingAttacks[56], 0x0203000000000000ull);
  BOOST_CHECK_EQUAL(kingAttacks[63], 0x40c0000000000000ull);

  BOOST_CHECK_EQUAL(kingAttacks[1], 0x0000000000000705ull);
  BOOST_CHECK_EQUAL(kingAttacks[28], 0x0000003828380000ull);
}

BOOST_AUTO_TEST_CASE(testKnightAttacks) {
  BOOST_CHECK_EQUAL(knightAttacks[0], 0x0000000000020400ull);
  BOOST_CHECK_EQUAL(knightAttacks[7], 0x0000000000402000ull);
  BOOST_CHECK_EQUAL(knightAttacks[56], 0x0004020000000000ull);
  BOOST_CHECK_EQUAL(knightAttacks[63], 0x0020400000000000ull);

  BOOST_CHECK_EQUAL(knightAttacks[23], 0x0000004020002040ull);
  BOOST_CHECK_EQUAL(knightAttacks[48], 0x0400040200000000ull);
  BOOST_CHECK_EQUAL(knightAttacks[44], 0x2844004428000000ull);
}

BOOST_AUTO_TEST_CASE(testIsoMoves) {
  BOOST_CHECK_EQUAL(isoMove[0][0], 0xfe);
  BOOST_CHECK_EQUAL(isoMove[0][63], 0x02);
  BOOST_CHECK_EQUAL(isoMove[0][4], 0x0e);
  BOOST_CHECK_EQUAL(isoMove[0][52], 0x0e);

  BOOST_CHECK_EQUAL(isoMove[4][0], 0xef);
  BOOST_CHECK_EQUAL(isoMove[4][63], 0x28);
  BOOST_CHECK_EQUAL(isoMove[4][55], 0x28);
  BOOST_CHECK_EQUAL(isoMove[4][36], 0x68);

  BOOST_CHECK_EQUAL(isoMove[7][0], 0x7f);
  BOOST_CHECK_EQUAL(isoMove[7][63], 0x40);
  BOOST_CHECK_EQUAL(isoMove[7][8], 0x70);
  BOOST_CHECK_EQUAL(isoMove[7][10], 0x70);
}

BOOST_AUTO_TEST_CASE(testFileConfigurations) {
  BOOST_CHECK_EQUAL(fileConfiguration[0], 0x0000000000000000ull);
  BOOST_CHECK_EQUAL(fileConfiguration[0xc6], 0x0001010000000101ull);
  BOOST_CHECK_EQUAL(fileConfiguration[0xff], 0x0101010101010101ull);
}

BOOST_AUTO_TEST_CASE(testDiagonals) {
  BOOST_CHECK_EQUAL(diagonal[0], 0x8040201008040201ull);
  BOOST_CHECK_EQUAL(diagonal[9], 0x8040201008040201ull);
  BOOST_CHECK_EQUAL(diagonal[18], 0x8040201008040201ull);
  BOOST_CHECK_EQUAL(diagonal[27], 0x8040201008040201ull);
  BOOST_CHECK_EQUAL(diagonal[36], 0x8040201008040201ull);
  BOOST_CHECK_EQUAL(diagonal[45], 0x8040201008040201ull);
  BOOST_CHECK_EQUAL(diagonal[54], 0x8040201008040201ull);
  BOOST_CHECK_EQUAL(diagonal[63], 0x8040201008040201ull);

  BOOST_CHECK_EQUAL(diagonal[16], 0x2010080402010000ull);
  BOOST_CHECK_EQUAL(diagonal[25], 0x2010080402010000ull);
  BOOST_CHECK_EQUAL(diagonal[34], 0x2010080402010000ull);
  BOOST_CHECK_EQUAL(diagonal[43], 0x2010080402010000ull);
  BOOST_CHECK_EQUAL(diagonal[52], 0x2010080402010000ull);
  BOOST_CHECK_EQUAL(diagonal[61], 0x2010080402010000ull);

  BOOST_CHECK_EQUAL(diagonal[4], 0x0000000080402010ull);
  BOOST_CHECK_EQUAL(diagonal[13], 0x0000000080402010ull);
  BOOST_CHECK_EQUAL(diagonal[22], 0x0000000080402010ull);
  BOOST_CHECK_EQUAL(diagonal[31], 0x0000000080402010ull);

  BOOST_CHECK_EQUAL(diagonal[7], 0x0000000000000080ull);
  BOOST_CHECK_EQUAL(diagonal[56], 0x0100000000000000ull);
}

BOOST_AUTO_TEST_CASE(testAntidiagonals) {
  BOOST_CHECK_EQUAL(antidiagonal[7], 0x0102040810204080ull);
  BOOST_CHECK_EQUAL(antidiagonal[14], 0x0102040810204080ull);
  BOOST_CHECK_EQUAL(antidiagonal[21], 0x0102040810204080ull);
  BOOST_CHECK_EQUAL(antidiagonal[28], 0x0102040810204080ull);
  BOOST_CHECK_EQUAL(antidiagonal[35], 0x0102040810204080ull);
  BOOST_CHECK_EQUAL(antidiagonal[42], 0x0102040810204080ull);
  BOOST_CHECK_EQUAL(antidiagonal[49], 0x0102040810204080ull);
  BOOST_CHECK_EQUAL(antidiagonal[56], 0x0102040810204080ull);

  BOOST_CHECK_EQUAL(antidiagonal[2], 0x0000000000010204ull);
  BOOST_CHECK_EQUAL(antidiagonal[9], 0x0000000000010204ull);
  BOOST_CHECK_EQUAL(antidiagonal[16], 0x0000000000010204ull);

  BOOST_CHECK_EQUAL(antidiagonal[31], 0x0810204080000000ull);
  BOOST_CHECK_EQUAL(antidiagonal[38], 0x0810204080000000ull);
  BOOST_CHECK_EQUAL(antidiagonal[45], 0x0810204080000000ull);
  BOOST_CHECK_EQUAL(antidiagonal[52], 0x0810204080000000ull);
  BOOST_CHECK_EQUAL(antidiagonal[59], 0x0810204080000000ull);

  BOOST_CHECK_EQUAL(antidiagonal[0], 0x0000000000000001ull);
  BOOST_CHECK_EQUAL(antidiagonal[63], 0x8000000000000000ull);
}

/************************* Tests for board.cpp *************************/

BOOST_AUTO_TEST_CASE(testFenToBoard) {
  Board b = fenToBoard(NEW_BOARD);
  BOOST_CHECK_EQUAL(b.bb[BB_WP], 0x000000000000ff00ull);
  BOOST_CHECK_EQUAL(b.bb[BB_WN], 0x0000000000000042ull);
  BOOST_CHECK_EQUAL(b.bb[BB_WB], 0x0000000000000024ull);
  BOOST_CHECK_EQUAL(b.bb[BB_WR], 0x0000000000000081ull);
  BOOST_CHECK_EQUAL(b.bb[BB_WQ], 0x0000000000000008ull);
  BOOST_CHECK_EQUAL(b.bb[BB_WK], 0x0000000000000010ull);
  BOOST_CHECK_EQUAL(b.bb[BB_WALL], 0x000000000000ffffull);

  BOOST_CHECK_EQUAL(b.bb[BB_BP], 0x00ff000000000000ull);
  BOOST_CHECK_EQUAL(b.bb[BB_BN], 0x4200000000000000ull);
  BOOST_CHECK_EQUAL(b.bb[BB_BB], 0x2400000000000000ull);
  BOOST_CHECK_EQUAL(b.bb[BB_BR], 0x8100000000000000ull);
  BOOST_CHECK_EQUAL(b.bb[BB_BQ], 0x0800000000000000ull);
  BOOST_CHECK_EQUAL(b.bb[BB_BK], 0x1000000000000000ull);
  BOOST_CHECK_EQUAL(b.bb[BB_BALL], 0xffff000000000000ull);

  BOOST_CHECK_EQUAL(b.bb[BB_EMPTY], 0x0000ffffffff0000ull);
  BOOST_CHECK_EQUAL(b.bb[BB_EP], 0ull);
  BOOST_CHECK_EQUAL(b.side, WHITE);

  // Position after 1. e4
  b = fenToBoard("rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1");
  BOOST_CHECK_EQUAL(b.bb[BB_WP], 0x000000001000ef00ull);
  BOOST_CHECK_EQUAL(b.bb[BB_EP], 0x0000000000100000);
  BOOST_CHECK_EQUAL(b.side, BLACK);
}

BOOST_AUTO_TEST_CASE(testMakeWhiteMove) {
  // Regular move: queen e4-h7
  Board b = fenToBoard("8/8/8/8/4Q3/8/8/8 w - - 42 1");
  Move m1 = { QUEEN, 28, 55, 0 };
  makeMove(&b, m1);

  BOOST_CHECK_EQUAL(b.bb[BB_WP], 0ull);
  BOOST_CHECK_EQUAL(b.bb[BB_WN], 0ull);
  BOOST_CHECK_EQUAL(b.bb[BB_WB], 0ull);
  BOOST_CHECK_EQUAL(b.bb[BB_WR], 0ull);
  BOOST_CHECK_EQUAL(b.bb[BB_WQ], 0x0080000000000000ull);
  BOOST_CHECK_EQUAL(b.bb[BB_WK], 0ull);
  BOOST_CHECK_EQUAL(b.bb[BB_WALL], 0x0080000000000000ull);

  BOOST_CHECK_EQUAL(b.bb[BB_BP], 0ull);
  BOOST_CHECK_EQUAL(b.bb[BB_BN], 0ull);
  BOOST_CHECK_EQUAL(b.bb[BB_BB], 0ull);
  BOOST_CHECK_EQUAL(b.bb[BB_BR], 0ull);
  BOOST_CHECK_EQUAL(b.bb[BB_BQ], 0ull);
  BOOST_CHECK_EQUAL(b.bb[BB_BK], 0ull);
  BOOST_CHECK_EQUAL(b.bb[BB_BALL], 0ull);

  BOOST_CHECK_EQUAL(b.bb[BB_EMPTY], 0xff7fffffffffffffull);
  BOOST_CHECK_EQUAL(b.bb[BB_EP], 0ull);
  BOOST_CHECK_EQUAL(b.side, BLACK);

  // Pawn push: h2h4
  b = fenToBoard("8/8/8/8/8/8/7P/8 w - - 42 1");
  Move m2 = { PAWN, 15, 31, 0 };
  makeMove(&b, m2);

  BOOST_CHECK_EQUAL(b.bb[BB_WP], 0x0000000080000000ull);
  BOOST_CHECK_EQUAL(b.bb[BB_WN], 0ull);
  BOOST_CHECK_EQUAL(b.bb[BB_WB], 0ull);
  BOOST_CHECK_EQUAL(b.bb[BB_WR], 0ull);
  BOOST_CHECK_EQUAL(b.bb[BB_WQ], 0ull);
  BOOST_CHECK_EQUAL(b.bb[BB_WK], 0ull);
  BOOST_CHECK_EQUAL(b.bb[BB_WALL], 0x0000000080000000ull);

  BOOST_CHECK_EQUAL(b.bb[BB_BP], 0ull);
  BOOST_CHECK_EQUAL(b.bb[BB_BN], 0ull);
  BOOST_CHECK_EQUAL(b.bb[BB_BB], 0ull);
  BOOST_CHECK_EQUAL(b.bb[BB_BR], 0ull);
  BOOST_CHECK_EQUAL(b.bb[BB_BQ], 0ull);
  BOOST_CHECK_EQUAL(b.bb[BB_BK], 0ull);
  BOOST_CHECK_EQUAL(b.bb[BB_BALL], 0ull);

  BOOST_CHECK_EQUAL(b.bb[BB_EMPTY], 0xffffffff7fffffffull);
  BOOST_CHECK_EQUAL(b.bb[BB_EP], 0x0000000000800000ull);
  BOOST_CHECK_EQUAL(b.side, BLACK);

  // Capture: Ne5xd3
  b = fenToBoard("8/8/8/4N3/8/3b4/8/8 w - - 42 1");
  Move m3 = { KNIGHT, 36, 19, 0 };
  makeMove(&b, m3);

  BOOST_CHECK_EQUAL(b.bb[BB_WP], 0ull);
  BOOST_CHECK_EQUAL(b.bb[BB_WN], 0x0000000000080000ull);
  BOOST_CHECK_EQUAL(b.bb[BB_WB], 0ull);
  BOOST_CHECK_EQUAL(b.bb[BB_WR], 0ull);
  BOOST_CHECK_EQUAL(b.bb[BB_WQ], 0ull);
  BOOST_CHECK_EQUAL(b.bb[BB_WK], 0ull);
  BOOST_CHECK_EQUAL(b.bb[BB_WALL], 0x0000000000080000ull);

  BOOST_CHECK_EQUAL(b.bb[BB_BP], 0ull);
  BOOST_CHECK_EQUAL(b.bb[BB_BN], 0ull);
  BOOST_CHECK_EQUAL(b.bb[BB_BB], 0ull);
  BOOST_CHECK_EQUAL(b.bb[BB_BR], 0ull);
  BOOST_CHECK_EQUAL(b.bb[BB_BQ], 0ull);
  BOOST_CHECK_EQUAL(b.bb[BB_BK], 0ull);
  BOOST_CHECK_EQUAL(b.bb[BB_BALL], 0ull);

  BOOST_CHECK_EQUAL(b.bb[BB_EMPTY], 0xfffffffffff7ffffull);
  BOOST_CHECK_EQUAL(b.bb[BB_EP], 0ull);
  BOOST_CHECK_EQUAL(b.side, BLACK);

  // Promotion: a7a8Q
  b = fenToBoard("8/P7/8/8/8/8/8/8 w - - 42 1");
  Move m4 = { PAWN, 48, 56, QUEEN };
  makeMove(&b, m4);

  BOOST_CHECK_EQUAL(b.bb[BB_WP], 0ull);
  BOOST_CHECK_EQUAL(b.bb[BB_WN], 0ull);
  BOOST_CHECK_EQUAL(b.bb[BB_WB], 0ull);
  BOOST_CHECK_EQUAL(b.bb[BB_WR], 0ull);
  BOOST_CHECK_EQUAL(b.bb[BB_WQ], 0x0100000000000000ull);
  BOOST_CHECK_EQUAL(b.bb[BB_WK], 0ull);
  BOOST_CHECK_EQUAL(b.bb[BB_WALL], 0x0100000000000000ull);

  BOOST_CHECK_EQUAL(b.bb[BB_BP], 0ull);
  BOOST_CHECK_EQUAL(b.bb[BB_BN], 0ull);
  BOOST_CHECK_EQUAL(b.bb[BB_BB], 0ull);
  BOOST_CHECK_EQUAL(b.bb[BB_BR], 0ull);
  BOOST_CHECK_EQUAL(b.bb[BB_BQ], 0ull);
  BOOST_CHECK_EQUAL(b.bb[BB_BK], 0ull);
  BOOST_CHECK_EQUAL(b.bb[BB_BALL], 0ull);

  BOOST_CHECK_EQUAL(b.bb[BB_EMPTY], 0xfeffffffffffffffull);
  BOOST_CHECK_EQUAL(b.bb[BB_EP], 0ull);
  BOOST_CHECK_EQUAL(b.side, BLACK);

  // En passant capture: c5xb6
  b = fenToBoard("8/8/8/1pP5/8/8/8/8 w - b6 42 1");
  Move m5 = { PAWN, 34, 41, 0 };
  makeMove(&b, m5);

  BOOST_CHECK_EQUAL(b.bb[BB_WP], 0x0000020000000000ull);
  BOOST_CHECK_EQUAL(b.bb[BB_WN], 0ull);
  BOOST_CHECK_EQUAL(b.bb[BB_WB], 0ull);
  BOOST_CHECK_EQUAL(b.bb[BB_WR], 0ull);
  BOOST_CHECK_EQUAL(b.bb[BB_WQ], 0ull);
  BOOST_CHECK_EQUAL(b.bb[BB_WK], 0ull);
  BOOST_CHECK_EQUAL(b.bb[BB_WALL], 0x0000020000000000ull);

  BOOST_CHECK_EQUAL(b.bb[BB_BP], 0ull);
  BOOST_CHECK_EQUAL(b.bb[BB_BN], 0ull);
  BOOST_CHECK_EQUAL(b.bb[BB_BB], 0ull);
  BOOST_CHECK_EQUAL(b.bb[BB_BR], 0ull);
  BOOST_CHECK_EQUAL(b.bb[BB_BQ], 0ull);
  BOOST_CHECK_EQUAL(b.bb[BB_BK], 0ull);
  BOOST_CHECK_EQUAL(b.bb[BB_BALL], 0ull);

  BOOST_CHECK_EQUAL(b.bb[BB_EMPTY], 0xfffffdffffffffffull);
  BOOST_CHECK_EQUAL(b.bb[BB_EP], 0ull);
  BOOST_CHECK_EQUAL(b.side, BLACK);
}

BOOST_AUTO_TEST_CASE(testMakeBlackMove) {
  // Regular move: queen e4-h7
  Board b = fenToBoard("8/8/8/8/4q3/8/8/8 b - - 42 1");
  Move m1 = { QUEEN, 28, 55, 0 };
  makeMove(&b, m1);

  BOOST_CHECK_EQUAL(b.bb[BB_BP], 0ull);
  BOOST_CHECK_EQUAL(b.bb[BB_BN], 0ull);
  BOOST_CHECK_EQUAL(b.bb[BB_BB], 0ull);
  BOOST_CHECK_EQUAL(b.bb[BB_BR], 0ull);
  BOOST_CHECK_EQUAL(b.bb[BB_BQ], 0x0080000000000000ull);
  BOOST_CHECK_EQUAL(b.bb[BB_BK], 0ull);
  BOOST_CHECK_EQUAL(b.bb[BB_BALL], 0x0080000000000000ull);

  BOOST_CHECK_EQUAL(b.bb[BB_WP], 0ull);
  BOOST_CHECK_EQUAL(b.bb[BB_WN], 0ull);
  BOOST_CHECK_EQUAL(b.bb[BB_WB], 0ull);
  BOOST_CHECK_EQUAL(b.bb[BB_WR], 0ull);
  BOOST_CHECK_EQUAL(b.bb[BB_WQ], 0ull);
  BOOST_CHECK_EQUAL(b.bb[BB_WK], 0ull);
  BOOST_CHECK_EQUAL(b.bb[BB_WALL], 0ull);

  BOOST_CHECK_EQUAL(b.bb[BB_EMPTY], 0xff7fffffffffffffull);
  BOOST_CHECK_EQUAL(b.bb[BB_EP], 0ull);
  BOOST_CHECK_EQUAL(b.side, WHITE);

  // Pawn push: h7h5
  b = fenToBoard("8/7p/8/8/8/8/8/8 b - - 42 1");
  Move m2 = { PAWN, 55, 39, 0 };
  makeMove(&b, m2);

  BOOST_CHECK_EQUAL(b.bb[BB_BP], 0x0000008000000000ull);
  BOOST_CHECK_EQUAL(b.bb[BB_BN], 0ull);
  BOOST_CHECK_EQUAL(b.bb[BB_BB], 0ull);
  BOOST_CHECK_EQUAL(b.bb[BB_BR], 0ull);
  BOOST_CHECK_EQUAL(b.bb[BB_BQ], 0ull);
  BOOST_CHECK_EQUAL(b.bb[BB_BK], 0ull);
  BOOST_CHECK_EQUAL(b.bb[BB_BALL], 0x0000008000000000ull);

  BOOST_CHECK_EQUAL(b.bb[BB_WP], 0ull);
  BOOST_CHECK_EQUAL(b.bb[BB_WN], 0ull);
  BOOST_CHECK_EQUAL(b.bb[BB_WB], 0ull);
  BOOST_CHECK_EQUAL(b.bb[BB_WR], 0ull);
  BOOST_CHECK_EQUAL(b.bb[BB_WQ], 0ull);
  BOOST_CHECK_EQUAL(b.bb[BB_WK], 0ull);
  BOOST_CHECK_EQUAL(b.bb[BB_WALL], 0ull);

  BOOST_CHECK_EQUAL(b.bb[BB_EMPTY], 0xffffff7fffffffffull);
  BOOST_CHECK_EQUAL(b.bb[BB_EP], 0x0000800000000000ull);
  BOOST_CHECK_EQUAL(b.side, WHITE);

  // Capture: Ne5xd3
  b = fenToBoard("8/8/8/4n3/8/3B4/8/8 b - - 42 1");
  Move m3 = { KNIGHT, 36, 19, 0 };
  makeMove(&b, m3);

  BOOST_CHECK_EQUAL(b.bb[BB_BP], 0ull);
  BOOST_CHECK_EQUAL(b.bb[BB_BN], 0x0000000000080000ull);
  BOOST_CHECK_EQUAL(b.bb[BB_BB], 0ull);
  BOOST_CHECK_EQUAL(b.bb[BB_BR], 0ull);
  BOOST_CHECK_EQUAL(b.bb[BB_BQ], 0ull);
  BOOST_CHECK_EQUAL(b.bb[BB_BK], 0ull);
  BOOST_CHECK_EQUAL(b.bb[BB_BALL], 0x0000000000080000ull);

  BOOST_CHECK_EQUAL(b.bb[BB_WP], 0ull);
  BOOST_CHECK_EQUAL(b.bb[BB_WN], 0ull);
  BOOST_CHECK_EQUAL(b.bb[BB_WB], 0ull);
  BOOST_CHECK_EQUAL(b.bb[BB_WR], 0ull);
  BOOST_CHECK_EQUAL(b.bb[BB_WQ], 0ull);
  BOOST_CHECK_EQUAL(b.bb[BB_WK], 0ull);
  BOOST_CHECK_EQUAL(b.bb[BB_WALL], 0ull);

  BOOST_CHECK_EQUAL(b.bb[BB_EMPTY], 0xfffffffffff7ffffull);
  BOOST_CHECK_EQUAL(b.bb[BB_EP], 0ull);
  BOOST_CHECK_EQUAL(b.side, WHITE);

  // Promotion: a2a1Q
  b = fenToBoard("8/8/8/8/8/8/p7/8 b - - 42 1");
  Move m4 = { PAWN, 8, 0, QUEEN };
  makeMove(&b, m4);

  BOOST_CHECK_EQUAL(b.bb[BB_BP], 0ull);
  BOOST_CHECK_EQUAL(b.bb[BB_BN], 0ull);
  BOOST_CHECK_EQUAL(b.bb[BB_BB], 0ull);
  BOOST_CHECK_EQUAL(b.bb[BB_BR], 0ull);
  BOOST_CHECK_EQUAL(b.bb[BB_BQ], 0x0000000000000001ull);
  BOOST_CHECK_EQUAL(b.bb[BB_BK], 0ull);
  BOOST_CHECK_EQUAL(b.bb[BB_BALL], 0x0000000000000001ull);

  BOOST_CHECK_EQUAL(b.bb[BB_WP], 0ull);
  BOOST_CHECK_EQUAL(b.bb[BB_WN], 0ull);
  BOOST_CHECK_EQUAL(b.bb[BB_WB], 0ull);
  BOOST_CHECK_EQUAL(b.bb[BB_WR], 0ull);
  BOOST_CHECK_EQUAL(b.bb[BB_WQ], 0ull);
  BOOST_CHECK_EQUAL(b.bb[BB_WK], 0ull);
  BOOST_CHECK_EQUAL(b.bb[BB_WALL], 0ull);

  BOOST_CHECK_EQUAL(b.bb[BB_EMPTY], 0xfffffffffffffffeull);
  BOOST_CHECK_EQUAL(b.bb[BB_EP], 0ull);
  BOOST_CHECK_EQUAL(b.side, WHITE);

  // En passant capture: c4xb3
  b = fenToBoard("8/8/8/8/1Pp5/8/8/8 b - b3 42 1");
  Move m5 = { PAWN, 26, 17, 0 };
  makeMove(&b, m5);

  BOOST_CHECK_EQUAL(b.bb[BB_BP], 0x0000000000020000ull);
  BOOST_CHECK_EQUAL(b.bb[BB_BN], 0ull);
  BOOST_CHECK_EQUAL(b.bb[BB_BB], 0ull);
  BOOST_CHECK_EQUAL(b.bb[BB_BR], 0ull);
  BOOST_CHECK_EQUAL(b.bb[BB_BQ], 0ull);
  BOOST_CHECK_EQUAL(b.bb[BB_BK], 0ull);
  BOOST_CHECK_EQUAL(b.bb[BB_BALL], 0x0000000000020000ull);

  BOOST_CHECK_EQUAL(b.bb[BB_WP], 0ull);
  BOOST_CHECK_EQUAL(b.bb[BB_WN], 0ull);
  BOOST_CHECK_EQUAL(b.bb[BB_WB], 0ull);
  BOOST_CHECK_EQUAL(b.bb[BB_WR], 0ull);
  BOOST_CHECK_EQUAL(b.bb[BB_WQ], 0ull);
  BOOST_CHECK_EQUAL(b.bb[BB_WK], 0ull);
  BOOST_CHECK_EQUAL(b.bb[BB_WALL], 0ull);

  BOOST_CHECK_EQUAL(b.bb[BB_EMPTY], 0xfffffffffffdffffull);
  BOOST_CHECK_EQUAL(b.bb[BB_EP], 0ull);
  BOOST_CHECK_EQUAL(b.side, WHITE);
}

/************************* Tests for movegen.cpp *************************/

BOOST_AUTO_TEST_CASE(testGetWhiteMoves) {
  Board b;
  Move m[100];
  string s[100];

  b = fenToBoard(NEW_BOARD);
  int numMoves = getAllMoves(&b, m);
  getAlgebraicNotation(&b, m, numMoves, s);
  const char *expected1[] = { "a3", "b3", "c3", "d3", "e3", "f3", "g3", "h3", "a4", "b4", "c4", "d4", "e4", "f4", "g4", "h4", "Na3", "Nc3", "Nf3", "Nh3" };
  BOOST_CHECK_EQUAL(numMoves, 20);
  BOOST_CHECK_EQUAL_COLLECTIONS(s, s + numMoves, expected1, expected1 + numMoves);

  b = fenToBoard("4q3/8/8/pP3N2/6K1/R5b1/5n1P/4Q3 w - a6 0 1");
  numMoves = getAllMoves(&b, m);
  getAlgebraicNotation(&b, m, numMoves, s);
  const char *expected2[] = { "hxg3", "bxa6", "Nxg3", "Rxg3", "Rxa5", "Qxe8", "Qxf2", "Qxa5", "Kxg3" };
  BOOST_CHECK_EQUAL(numMoves, 9);
  BOOST_CHECK_EQUAL_COLLECTIONS(s, s + numMoves, expected2, expected2 + numMoves);

  b = fenToBoard("8/4P3/8/8/8/8/q7/8 w - - 0 1");
  numMoves = getAllMoves(&b, m);
  getAlgebraicNotation(&b, m, numMoves, s);
  const char *expected3[] = { "e8=N", "e8=B", "e8=R", "e8=Q", "e8=K" };
  BOOST_CHECK_EQUAL(numMoves, 5);
  BOOST_CHECK_EQUAL_COLLECTIONS(s, s + numMoves, expected3, expected3 + numMoves);

  // Both the c and e white pawns can capture en passant
  b = fenToBoard("8/8/8/2PpP3/8/8/8/8 w - d6 0 1");
  numMoves = getAllMoves(&b, m);
  getAlgebraicNotation(&b, m, numMoves, s);
  const char *expected4[] = { "cxd6", "exd6" };
  BOOST_CHECK_EQUAL(numMoves, 2);
  BOOST_CHECK_EQUAL_COLLECTIONS(s, s + numMoves, expected4, expected4 + numMoves);

  // Resolving ambiguities
  b = fenToBoard("8/2q4q/8/7q/1N6/8/1N3N2/6N1 w - - 0 1");
  numMoves = getAllMoves(&b, m);
  getAlgebraicNotation(&b, m, numMoves, s);
  const char *expected5[] = { "Ne2", "Nf3", "Ngh3", "Nbd1", "Nb2d3", "Na4", "Nc4", "Nfd1", "Nh1", "Nfd3", "Nfh3", "Ne4", "Ng4", "Na2", "Nc2", "N4d3", "Nd5", "Na6", "Nc6" };
  BOOST_CHECK_EQUAL(numMoves, 19);
  BOOST_CHECK_EQUAL_COLLECTIONS(s, s + numMoves, expected5, expected5 + numMoves);

  // Resolving ambiguities
  b = fenToBoard("8/2Q4Q/8/7Q/1n6/8/1n3n2/6n1 w - - 0 1");
  numMoves = getAllMoves(&b, m);
  getAlgebraicNotation(&b, m, numMoves, s);
  const char *expected6[] = { "Qha5", "Qb5", "Qhc5", "Qd5", "Qhe5", "Q5f5", "Qg5", "Qa7", "Qb7", "Qcd7", "Qce7", "Qcf7", "Qcg7", "Qhd7", "Qhe7", "Qh7f7",
    "Qhg7", "Qh1", "Qhh2", "Qh3", "Qh4", "Q5h6", "Qc1", "Qcc2", "Qc3", "Qc4", "Qcc5", "Qc6", "Qc8", "Q7h6", "Qh8", "Qd1", "Qe2", "Qf3", "Qg4", "Qca5",
    "Qb6", "Qd8", "Qb1", "Qhc2", "Qd3", "Qe4", "Q7f5", "Q7g6", "Q5g6", "Q5f7", "Qe8", "Qch2", "Qg3", "Qf4", "Qce5", "Qd6", "Qb8", "Qg8", };
  BOOST_CHECK_EQUAL(numMoves, 54);
  BOOST_CHECK_EQUAL_COLLECTIONS(s, s + numMoves, expected6, expected6 + numMoves);

  // Diagonal and antidiagonal moves
  b = fenToBoard("8/3Q4/1B6/8/4B1B1/8/3B4/k7 w - - 0 1");
  numMoves = getAllMoves(&b, m);
  getAlgebraicNotation(&b, m, numMoves, s);
  const char *expected7[] = { "Bc1", "Bde3", "Bf4", "Bg5", "Bh6", "Bb1", "Bc2", "Bd3", "Bef5", "Bg6", "Bh7", "Bd1", "Be2", "Bgf3", "Bh5", "Bba5", "Bc7",
    "Bd8", "Be1", "Bc3", "Bb4", "Bda5", "Bh1", "Bg2", "Bef3", "Bd5", "Bc6", "Bb7", "Ba8", "Bh3", "Bgf5", "Be6", "Bg1", "Bf2", "Bbe3", "Bd4", "Bc5", "Ba7",
    "Qa7", "Qb7", "Qc7", "Qe7", "Qf7", "Qg7", "Qh7", "Qd3", "Qd4", "Qd5", "Qd6", "Qd8", "Qa4", "Qb5", "Qc6", "Qe8", "Qf5", "Qe6", "Qc8" };
  BOOST_CHECK_EQUAL(numMoves, 57);
  BOOST_CHECK_EQUAL_COLLECTIONS(s, s + numMoves, expected7, expected7 + numMoves);
}

BOOST_AUTO_TEST_CASE(testGetBlackMoves) {
  Board b;
  Move m[100];
  string s[100];

  b = fenToBoard("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR b KQkq - 0 1");
  int numMoves = getAllMoves(&b, m);
  getAlgebraicNotation(&b, m, numMoves, s);
  const char *expected1[] = { "a6", "b6", "c6", "d6", "e6", "f6", "g6", "h6", "a5", "b5", "c5", "d5", "e5", "f5", "g5", "h5", "Na6", "Nc6", "Nf6", "Nh6" };
  BOOST_CHECK_EQUAL(numMoves, 20);
  BOOST_CHECK_EQUAL_COLLECTIONS(s, s + numMoves, expected1, expected1 + numMoves);

  b = fenToBoard("4q3/5N1p/r5B1/6k1/Pp3n2/8/8/4Q3 b - a3 0 1");
  numMoves = getAllMoves(&b, m);
  getAlgebraicNotation(&b, m, numMoves, s);
  const char *expected2[] = { "bxa3", "hxg6", "Nxg6", "Rxg6", "Rxa4", "Qxe1", "Qxa4", "Qxf7", "Kxg6" };
  BOOST_CHECK_EQUAL(numMoves, 9);
  BOOST_CHECK_EQUAL_COLLECTIONS(s, s + numMoves, expected2, expected2 + numMoves);

  b = fenToBoard("8/Q7/8/8/8/8/4p3/8 b - - 0 1");
  numMoves = getAllMoves(&b, m);
  getAlgebraicNotation(&b, m, numMoves, s);
  const char *expected3[] = { "e1=N", "e1=B", "e1=R", "e1=Q", "e1=K" };
  BOOST_CHECK_EQUAL(numMoves, 5);
  BOOST_CHECK_EQUAL_COLLECTIONS(s, s + numMoves, expected3, expected3 + numMoves);

  // Both the c and e white pawns can capture en passant
  b = fenToBoard("8/8/8/8/2pPp3/8/8/8 b - d3 0 1");
  numMoves = getAllMoves(&b, m);
  getAlgebraicNotation(&b, m, numMoves, s);
  const char *expected4[] = { "cxd3", "exd3" };
  BOOST_CHECK_EQUAL(numMoves, 2);
  BOOST_CHECK_EQUAL_COLLECTIONS(s, s + numMoves, expected4, expected4 + numMoves);

  // Resolving ambiguities
  b = fenToBoard("6n1/1n3n2/8/1n6/7Q/8/2Q4Q/8 b - - 0 1");
  numMoves = getAllMoves(&b, m);
  getAlgebraicNotation(&b, m, numMoves, s);
  const char *expected5[] = { "Na3", "Nc3", "Nd4", "N5d6", "Na7", "Nc7", "Na5", "Nc5", "Nb7d6", "Nbd8", "Ne5", "Ng5", "Nfd6", "Nfh6", "Nfd8", "Nh8", "Nf6", "Ngh6", "Ne7" };
  BOOST_CHECK_EQUAL(numMoves, 19);
  BOOST_CHECK_EQUAL_COLLECTIONS(s, s + numMoves, expected5, expected5 + numMoves);

  // Resolving ambiguities
  b = fenToBoard("6N1/1N3N2/8/1N6/7q/8/2q4q/8 b - - 0 1");
  numMoves = getAllMoves(&b, m);
  getAlgebraicNotation(&b, m, numMoves, s);
  const char *expected6[] = { "Qa2", "Qb2", "Qcd2", "Qce2", "Qcf2", "Qcg2", "Qhd2", "Qhe2", "Qh2f2", "Qhg2", "Qha4", "Qb4", "Qhc4", "Qd4", "Qhe4",
    "Q4f4", "Qg4", "Qc1", "Qc3", "Qcc4", "Qc5", "Qc6", "Qcc7", "Qc8", "Qh1", "Q2h3", "Q4h3", "Qh5", "Qh6", "Qhh7", "Qh8", "Qb1", "Qd3", "Qce4", "Qf5",
    "Qg6", "Qch7", "Qg1", "Qe1", "Q4f2", "Q4g3", "Qd1", "Qb3", "Qca4", "Q2g3", "Q2f4", "Qe5", "Qd6", "Qhc7", "Qb8", "Qg5", "Qf6", "Qe7", "Qd8", };
  BOOST_CHECK_EQUAL(numMoves, 54);
  BOOST_CHECK_EQUAL_COLLECTIONS(s, s + numMoves, expected6, expected6 + numMoves);

  // Diagonal and antidiagonal moves
  b = fenToBoard("K7/3b4/8/4b1b1/8/1b6/3q4/8 b - - 0 1");
  numMoves = getAllMoves(&b, m);
  getAlgebraicNotation(&b, m, numMoves, s);
  const char *expected7[] = { "Ba2", "Bc4", "Bd5", "Bbe6", "Bf7", "Bg8", "Ba1", "Bb2", "Bc3", "Bd4", "Bef6", "Bg7", "Bh8", "Be3", "Bgf4", "Bh6", "Bda4",
    "Bb5", "Bc6", "Be8", "Bd1", "Bc2", "Bba4", "Bh2", "Bg3", "Bef4", "Bd6", "Bc7", "Bb8", "Bh4", "Bgf6", "Be7", "Bd8", "Bh3", "Bg4", "Bf5", "Bde6", "Bc8",
    "Qa2", "Qb2", "Qc2", "Qe2", "Qf2", "Qg2", "Qh2", "Qd1", "Qd3", "Qd4", "Qd5", "Qd6", "Qc1", "Qe3", "Qf4", "Qe1", "Qc3", "Qb4", "Qa5" };
  BOOST_CHECK_EQUAL(numMoves, 57);
  BOOST_CHECK_EQUAL_COLLECTIONS(s, s + numMoves, expected7, expected7 + numMoves);
}
