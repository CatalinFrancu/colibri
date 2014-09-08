#define BOOST_TEST_MODULE colibriTest
#include <boost/test/included/unit_test.hpp>
#include <unistd.h>
#include "bitmanip.h"
#include "egtb.h"
#include "fileutil.h"
#include "lrucache.h"
#include "movegen.h"
#include "precomp.h"
#include "stringutil.h"

/************************* Tests for bitmanip.cpp *************************/

BOOST_AUTO_TEST_CASE(testSgn) {
  BOOST_CHECK_EQUAL(sgn(0), 0);
  BOOST_CHECK_EQUAL(sgn(23), 1);
  BOOST_CHECK_EQUAL(sgn(-18), -1);
}

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
  BOOST_CHECK_EQUAL(ctz(0x0fc0100000000000ull), 44);
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

BOOST_AUTO_TEST_CASE(testRotateSquare) {
  precomputeAll();
  BOOST_CHECK_EQUAL(rotateSquare[11][ORI_NORMAL], 11);
  BOOST_CHECK_EQUAL(rotateSquare[11][ORI_ROT_CCW], 30);
  BOOST_CHECK_EQUAL(rotateSquare[11][ORI_ROT_180], 52);
  BOOST_CHECK_EQUAL(rotateSquare[11][ORI_ROT_CW], 33);
  BOOST_CHECK_EQUAL(rotateSquare[11][ORI_FLIP_NS], 51);
  BOOST_CHECK_EQUAL(rotateSquare[11][ORI_FLIP_DIAG], 25);
  BOOST_CHECK_EQUAL(rotateSquare[11][ORI_FLIP_EW], 12);
  BOOST_CHECK_EQUAL(rotateSquare[11][ORI_FLIP_ANTIDIAG], 38);
}

BOOST_AUTO_TEST_CASE(testNChooseK) {
  BOOST_CHECK_EQUAL(choose[4][1], 4);
  if (EGTB_MEN > 3) {
    BOOST_CHECK_EQUAL(choose[6][2], 15);
    BOOST_CHECK_EQUAL(choose[64][3], 41664);
  }
  if (EGTB_MEN > 4) {
    BOOST_CHECK_EQUAL(choose[8][4], 70);
    BOOST_CHECK_EQUAL(choose[64][4], 635376);
  }
}

BOOST_AUTO_TEST_CASE(testRankCombination) {
  BOOST_CHECK_EQUAL(rankCombination(0x8000000000000000ull, 0), 63);
  if (EGTB_MEN > 3) {
    BOOST_CHECK_EQUAL(rankCombination(0x000000000000000full, 0), 0);
    BOOST_CHECK_EQUAL(rankCombination(0x0000010000400000ull, 0), 802);
    BOOST_CHECK_EQUAL(rankCombination(0x0000000001000040ull, 0), 282);
    BOOST_CHECK_EQUAL(rankCombination(0x0000000000000007ull, 0), 0);

    // Now some test cases where some squares are already taken, as indicated by the third argument
    BOOST_CHECK_EQUAL(rankCombination(0x0000000000000c00ull, 0x000000000000000full), 27);
    BOOST_CHECK_EQUAL(rankCombination(0x24ull, 0x11ull), 4);
  }
  if (EGTB_MEN > 4) {
    BOOST_CHECK_EQUAL(rankCombination(0xf000000000000000ull, 0), 635375);
    BOOST_CHECK_EQUAL(rankCombination(0x0000f00000000000ull, 0), 194579);

    // Now some test cases where some squares are already taken, as indicated by the third argument
    BOOST_CHECK_EQUAL(rankCombination(0x000000000000000full, 0x0000000010101080ull), 0);
    BOOST_CHECK_EQUAL(rankCombination(0x0f00000000000000ull, 0xf0f0f0f0f0f0f0f0ull), 35959);
  }
}

BOOST_AUTO_TEST_CASE(testUnrankCombination) {
  BOOST_CHECK_EQUAL(unrankCombination(63, 1, 0), 0x8000000000000000ull);
  if (EGTB_MEN > 3) {
    BOOST_CHECK_EQUAL(unrankCombination(802, 2, 0), 0x0000010000400000ull);
    BOOST_CHECK_EQUAL(unrankCombination(282, 2, 0), 0x0000000001000040ull);
    BOOST_CHECK_EQUAL(unrankCombination(0, 3, 0), 0x0000000000000007ull);

    BOOST_CHECK_EQUAL(unrankCombination(0, 2, 0x0000000000000003ull), 0x000000000000000cull);
    BOOST_CHECK_EQUAL(unrankCombination(1890, 2, 0x0000000000000003ull), 0xc000000000000000ull);
    BOOST_CHECK_EQUAL(unrankCombination(40, 2, 0x0000000010000400ull), 0x0000000000000210ull);
    BOOST_CHECK_EQUAL(unrankCombination(222, 2, 0x0000000010000400ull), 0x0000000000402000ull);
    BOOST_CHECK_EQUAL(unrankCombination(1275, 2, 0x0000000010000400ull), 0x0020000000000001ull);
    BOOST_CHECK_EQUAL(unrankCombination(1302, 2, 0x0000000010000400ull), 0x0020000020000000ull);
    BOOST_CHECK_EQUAL(unrankCombination(54, 2, 0x0000000000001800ull), 0x0000000000000600ull);
    BOOST_CHECK_EQUAL(unrankCombination(55, 2, 0x0000000000001800ull), 0x0000000000002001ull);
  }
  if (EGTB_MEN > 4) {
    BOOST_CHECK_EQUAL(unrankCombination(0, 4, 0), 0x000000000000000full);
    BOOST_CHECK_EQUAL(unrankCombination(635375, 4, 0), 0xf000000000000000ull);
    BOOST_CHECK_EQUAL(unrankCombination(194579, 4, 0), 0x0000f00000000000ull);
  }
}

BOOST_AUTO_TEST_CASE(testCanonical) {
  BOOST_CHECK_EQUAL(numCanonical64[1], 10);
  BOOST_CHECK_EQUAL(numCanonical64[2], 278);
  BOOST_CHECK_EQUAL(numCanonical48[1], 24);
  BOOST_CHECK_EQUAL(numCanonical48[2], 576);
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

BOOST_AUTO_TEST_CASE(testGetPieceCount) {
  Board *b = fenToBoard(NEW_BOARD);
  BOOST_CHECK_EQUAL(getPieceCount(b), 32);
  free(b);

  b = fenToBoard("8/8/8/4N3/8/3b4/8/8 w - - 42 1");
  BOOST_CHECK_EQUAL(getPieceCount(b), 2);
  free(b);
}

BOOST_AUTO_TEST_CASE(testSwitchSides) {
  Board *b = fenToBoard("8/8/3b2N1/8/1P6/4p3/8/8 b - b3 0 0");
  changeSides(b);

  BOOST_CHECK_EQUAL(b->bb[BB_WP], 0x0000100000000000ull);
  BOOST_CHECK_EQUAL(b->bb[BB_WN], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_WB], 0x0000000000080000ull);
  BOOST_CHECK_EQUAL(b->bb[BB_WR], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_WQ], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_WK], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_WALL], 0x0000100000080000ull);

  BOOST_CHECK_EQUAL(b->bb[BB_BP], 0x0000000200000000ull);
  BOOST_CHECK_EQUAL(b->bb[BB_BN], 0x0000000000400000ull);
  BOOST_CHECK_EQUAL(b->bb[BB_BB], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_BR], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_BQ], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_BK], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_BALL], 0x0000000200400000ull);

  BOOST_CHECK_EQUAL(b->bb[BB_EMPTY], 0xffffeffdffb7ffffull);
  BOOST_CHECK_EQUAL(b->bb[BB_EP], 0x0000020000000000ull);
  BOOST_CHECK_EQUAL(b->side, WHITE);
  free(b);
}

BOOST_AUTO_TEST_CASE(testEpCapturePossible) {
  Board *b = fenToBoard("8/8/8/pP6/8/8/8/8 w - a6 0 0");
  BOOST_CHECK_EQUAL(epCapturePossible(b), true);
  free(b);

  b = fenToBoard("8/8/8/p7/7P/8/8/8 w - a6 0 0");
  BOOST_CHECK_EQUAL(epCapturePossible(b), false);
  free(b);
}

BOOST_AUTO_TEST_CASE(testCanonicalizeBoard) {
  PieceSet ps[EGTB_MEN];
  int nps = comboToPieceSets("KQvN", ps);
  Board *b = fenToBoard("8/8/4n3/8/8/Q7/5K2/8 w - - 0 0");
  canonicalizeBoard(ps, nps, b);

  BOOST_CHECK_EQUAL(b->bb[BB_WP], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_WN], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_WB], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_WR], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_WQ], 0x0000000000000004ull);
  BOOST_CHECK_EQUAL(b->bb[BB_WK], 0x0000020000000000ull);
  BOOST_CHECK_EQUAL(b->bb[BB_WALL], 0x0000020000000004ull);

  BOOST_CHECK_EQUAL(b->bb[BB_BP], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_BN], 0x0000002000000000ull);
  BOOST_CHECK_EQUAL(b->bb[BB_BB], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_BR], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_BQ], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_BK], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_BALL], 0x0000002000000000ull);

  BOOST_CHECK_EQUAL(b->bb[BB_EMPTY], 0xfffffddffffffffbull);
  BOOST_CHECK_EQUAL(b->bb[BB_EP], 0ull);
  BOOST_CHECK_EQUAL(b->side, WHITE);
  free(b);

  // En passant case
  nps = comboToPieceSets("NNPvPP", ps);
  b = fenToBoard("8/2p5/8/1N6/5pP1/8/8/5N2 b - g3 0 0");
  canonicalizeBoard(ps, nps, b);

  BOOST_CHECK_EQUAL(b->bb[BB_WP], 0x0000000002000000ull);
  BOOST_CHECK_EQUAL(b->bb[BB_WN], 0x0000004000000004ull);
  BOOST_CHECK_EQUAL(b->bb[BB_WB], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_WR], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_WQ], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_WK], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_WALL], 0x0000004002000004ull);

  BOOST_CHECK_EQUAL(b->bb[BB_BP], 0x0020000004000000ull);
  BOOST_CHECK_EQUAL(b->bb[BB_BN], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_BB], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_BR], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_BQ], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_BK], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_BALL], 0x0020000004000000ull);

  BOOST_CHECK_EQUAL(b->bb[BB_EMPTY], 0xffdfffbff9fffffbull);
  BOOST_CHECK_EQUAL(b->bb[BB_EP], 0x0000000000020000ull);
  BOOST_CHECK_EQUAL(b->side, BLACK);
  free(b);
}

BOOST_AUTO_TEST_CASE(testFenToBoard) {
  Board *b = fenToBoard(NEW_BOARD);
  BOOST_CHECK_EQUAL(b->bb[BB_WP], 0x000000000000ff00ull);
  BOOST_CHECK_EQUAL(b->bb[BB_WN], 0x0000000000000042ull);
  BOOST_CHECK_EQUAL(b->bb[BB_WB], 0x0000000000000024ull);
  BOOST_CHECK_EQUAL(b->bb[BB_WR], 0x0000000000000081ull);
  BOOST_CHECK_EQUAL(b->bb[BB_WQ], 0x0000000000000008ull);
  BOOST_CHECK_EQUAL(b->bb[BB_WK], 0x0000000000000010ull);
  BOOST_CHECK_EQUAL(b->bb[BB_WALL], 0x000000000000ffffull);

  BOOST_CHECK_EQUAL(b->bb[BB_BP], 0x00ff000000000000ull);
  BOOST_CHECK_EQUAL(b->bb[BB_BN], 0x4200000000000000ull);
  BOOST_CHECK_EQUAL(b->bb[BB_BB], 0x2400000000000000ull);
  BOOST_CHECK_EQUAL(b->bb[BB_BR], 0x8100000000000000ull);
  BOOST_CHECK_EQUAL(b->bb[BB_BQ], 0x0800000000000000ull);
  BOOST_CHECK_EQUAL(b->bb[BB_BK], 0x1000000000000000ull);
  BOOST_CHECK_EQUAL(b->bb[BB_BALL], 0xffff000000000000ull);

  BOOST_CHECK_EQUAL(b->bb[BB_EMPTY], 0x0000ffffffff0000ull);
  BOOST_CHECK_EQUAL(b->bb[BB_EP], 0ull);
  BOOST_CHECK_EQUAL(b->side, WHITE);
  free(b);

  // Position after 1. e4
  b = fenToBoard("rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b - e3 0 1");
  BOOST_CHECK_EQUAL(b->bb[BB_WP], 0x000000001000ef00ull);
  BOOST_CHECK_EQUAL(b->bb[BB_EP], 0x0000000000100000);
  BOOST_CHECK_EQUAL(b->side, BLACK);
  free(b);

  // All kinds of wrong things
  BOOST_CHECK(!fenToBoard("")); // No data
  BOOST_CHECK(!fenToBoard("3b")); // Incomplete data
  BOOST_CHECK(!fenToBoard("rnbqkbnr/ppZppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b - e3 0 1")); // Illegal piece names
  BOOST_CHECK(!fenToBoard("rnbqkbnr/pppp3pppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b - e3 0 1")); // Too many squares on one rank
  BOOST_CHECK(!fenToBoard("rnbqkbnr/ppppppp3/8/8/4P3/8/PPPP1PPP/RNBQKBNR b - e3 0 1")); // Same, but the culprit is a number
  BOOST_CHECK(!fenToBoard("rnbqkbnr/pp2/8/8/4P3/8/PPPP1PPP/RNBQKBNR b - e3 0 1")); // Too few squares on one rank
  BOOST_CHECK(!fenToBoard("rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR/ppppPPPP b - e3 0 1")); // Too many ranks
  BOOST_CHECK(!fenToBoard("rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR q - e3 0 1")); // Illegal side name
  BOOST_CHECK(!fenToBoard("rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1")); // No castling availability please
  BOOST_CHECK(!fenToBoard("rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNpQKBNR b - e3 0 1")); // Pawn on the first rank
  BOOST_CHECK(!fenToBoard("rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b - z3 0 1")); // Incorrect en passant square name
  BOOST_CHECK(!fenToBoard("rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b - e5 0 1")); // En passant square on illegal rank
  BOOST_CHECK(!fenToBoard("rnbqkbnr/pppppppp/8/8/8/4P3/PPPP1PPP/RNBQKBNR b - e3 0 1")); // En passant square is not empty
  BOOST_CHECK(!fenToBoard("rnbqkbnr/pppppppp/8/8/4P3/8/PPPPNPPP/RNBQKBNR b - e3 0 1")); // Square behind en passant square is not empty
  BOOST_CHECK(!fenToBoard("rnbqkbnr/pppppppp/8/8/8/8/PPPP1PPP/RNBQKBNR b - e3 0 1")); // No pawn in front of en passant square
}

BOOST_AUTO_TEST_CASE(testBoardToFen) {
  const char *s1 = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w - - 0 0";
  Board *b = fenToBoard(s1);
  BOOST_CHECK_EQUAL(boardToFen(b), s1);
  free(b);

  // Position after 1. e4
  const char *s2 = "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b - e3 0 0";
  b = fenToBoard(s2);
  BOOST_CHECK_EQUAL(boardToFen(b), s2);
  free(b);
}

BOOST_AUTO_TEST_CASE(testMakeWhiteMove) {
  // Regular move: queen e4-h7
  Board *b = fenToBoard("8/8/8/8/4Q3/8/8/8 w - - 42 1");
  Move m1 = { QUEEN, 28, 55, 0 };
  makeMove(b, m1);

  BOOST_CHECK_EQUAL(b->bb[BB_WP], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_WN], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_WB], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_WR], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_WQ], 0x0080000000000000ull);
  BOOST_CHECK_EQUAL(b->bb[BB_WK], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_WALL], 0x0080000000000000ull);

  BOOST_CHECK_EQUAL(b->bb[BB_BP], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_BN], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_BB], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_BR], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_BQ], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_BK], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_BALL], 0ull);

  BOOST_CHECK_EQUAL(b->bb[BB_EMPTY], 0xff7fffffffffffffull);
  BOOST_CHECK_EQUAL(b->bb[BB_EP], 0ull);
  BOOST_CHECK_EQUAL(b->side, BLACK);
  free(b);

  // Pawn push: h2h4
  b = fenToBoard("8/8/8/8/8/8/7P/8 w - - 42 1");
  Move m2 = { PAWN, 15, 31, 0 };
  makeMove(b, m2);

  BOOST_CHECK_EQUAL(b->bb[BB_WP], 0x0000000080000000ull);
  BOOST_CHECK_EQUAL(b->bb[BB_WN], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_WB], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_WR], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_WQ], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_WK], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_WALL], 0x0000000080000000ull);

  BOOST_CHECK_EQUAL(b->bb[BB_BP], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_BN], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_BB], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_BR], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_BQ], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_BK], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_BALL], 0ull);

  BOOST_CHECK_EQUAL(b->bb[BB_EMPTY], 0xffffffff7fffffffull);
  BOOST_CHECK_EQUAL(b->bb[BB_EP], 0x0000000000800000ull);
  BOOST_CHECK_EQUAL(b->side, BLACK);
  free(b);

  // Capture: Ne5xd3
  b = fenToBoard("8/8/8/4N3/8/3b4/8/8 w - - 42 1");
  Move m3 = { KNIGHT, 36, 19, 0 };
  makeMove(b, m3);

  BOOST_CHECK_EQUAL(b->bb[BB_WP], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_WN], 0x0000000000080000ull);
  BOOST_CHECK_EQUAL(b->bb[BB_WB], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_WR], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_WQ], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_WK], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_WALL], 0x0000000000080000ull);

  BOOST_CHECK_EQUAL(b->bb[BB_BP], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_BN], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_BB], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_BR], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_BQ], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_BK], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_BALL], 0ull);

  BOOST_CHECK_EQUAL(b->bb[BB_EMPTY], 0xfffffffffff7ffffull);
  BOOST_CHECK_EQUAL(b->bb[BB_EP], 0ull);
  BOOST_CHECK_EQUAL(b->side, BLACK);
  free(b);

  // Promotion: a7a8Q
  b = fenToBoard("8/P7/8/8/8/8/8/8 w - - 42 1");
  Move m4 = { PAWN, 48, 56, QUEEN };
  makeMove(b, m4);

  BOOST_CHECK_EQUAL(b->bb[BB_WP], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_WN], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_WB], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_WR], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_WQ], 0x0100000000000000ull);
  BOOST_CHECK_EQUAL(b->bb[BB_WK], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_WALL], 0x0100000000000000ull);

  BOOST_CHECK_EQUAL(b->bb[BB_BP], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_BN], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_BB], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_BR], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_BQ], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_BK], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_BALL], 0ull);

  BOOST_CHECK_EQUAL(b->bb[BB_EMPTY], 0xfeffffffffffffffull);
  BOOST_CHECK_EQUAL(b->bb[BB_EP], 0ull);
  BOOST_CHECK_EQUAL(b->side, BLACK);
  free(b);

  // En passant capture: c5xb6
  b = fenToBoard("8/8/8/1pP5/8/8/8/8 w - b6 42 1");
  Move m5 = { PAWN, 34, 41, 0 };
  makeMove(b, m5);

  BOOST_CHECK_EQUAL(b->bb[BB_WP], 0x0000020000000000ull);
  BOOST_CHECK_EQUAL(b->bb[BB_WN], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_WB], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_WR], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_WQ], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_WK], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_WALL], 0x0000020000000000ull);

  BOOST_CHECK_EQUAL(b->bb[BB_BP], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_BN], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_BB], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_BR], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_BQ], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_BK], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_BALL], 0ull);

  BOOST_CHECK_EQUAL(b->bb[BB_EMPTY], 0xfffffdffffffffffull);
  BOOST_CHECK_EQUAL(b->bb[BB_EP], 0ull);
  BOOST_CHECK_EQUAL(b->side, BLACK);
  free(b);
}

BOOST_AUTO_TEST_CASE(testMakeBlackMove) {
  // Regular move: queen e4-h7
  Board *b = fenToBoard("8/8/8/8/4q3/8/8/8 b - - 42 1");
  Move m1 = { QUEEN, 28, 55, 0 };
  makeMove(b, m1);

  BOOST_CHECK_EQUAL(b->bb[BB_BP], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_BN], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_BB], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_BR], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_BQ], 0x0080000000000000ull);
  BOOST_CHECK_EQUAL(b->bb[BB_BK], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_BALL], 0x0080000000000000ull);

  BOOST_CHECK_EQUAL(b->bb[BB_WP], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_WN], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_WB], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_WR], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_WQ], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_WK], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_WALL], 0ull);

  BOOST_CHECK_EQUAL(b->bb[BB_EMPTY], 0xff7fffffffffffffull);
  BOOST_CHECK_EQUAL(b->bb[BB_EP], 0ull);
  BOOST_CHECK_EQUAL(b->side, WHITE);
  free(b);

  // Pawn push: h7h5
  b = fenToBoard("8/7p/8/8/8/8/8/8 b - - 42 1");
  Move m2 = { PAWN, 55, 39, 0 };
  makeMove(b, m2);

  BOOST_CHECK_EQUAL(b->bb[BB_BP], 0x0000008000000000ull);
  BOOST_CHECK_EQUAL(b->bb[BB_BN], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_BB], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_BR], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_BQ], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_BK], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_BALL], 0x0000008000000000ull);

  BOOST_CHECK_EQUAL(b->bb[BB_WP], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_WN], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_WB], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_WR], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_WQ], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_WK], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_WALL], 0ull);

  BOOST_CHECK_EQUAL(b->bb[BB_EMPTY], 0xffffff7fffffffffull);
  BOOST_CHECK_EQUAL(b->bb[BB_EP], 0x0000800000000000ull);
  BOOST_CHECK_EQUAL(b->side, WHITE);
  free(b);

  // Capture: Ne5xd3
  b = fenToBoard("8/8/8/4n3/8/3B4/8/8 b - - 42 1");
  Move m3 = { KNIGHT, 36, 19, 0 };
  makeMove(b, m3);

  BOOST_CHECK_EQUAL(b->bb[BB_BP], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_BN], 0x0000000000080000ull);
  BOOST_CHECK_EQUAL(b->bb[BB_BB], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_BR], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_BQ], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_BK], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_BALL], 0x0000000000080000ull);

  BOOST_CHECK_EQUAL(b->bb[BB_WP], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_WN], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_WB], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_WR], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_WQ], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_WK], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_WALL], 0ull);

  BOOST_CHECK_EQUAL(b->bb[BB_EMPTY], 0xfffffffffff7ffffull);
  BOOST_CHECK_EQUAL(b->bb[BB_EP], 0ull);
  BOOST_CHECK_EQUAL(b->side, WHITE);
  free(b);

  // Promotion: a2a1Q
  b = fenToBoard("8/8/8/8/8/8/p7/8 b - - 42 1");
  Move m4 = { PAWN, 8, 0, QUEEN };
  makeMove(b, m4);

  BOOST_CHECK_EQUAL(b->bb[BB_BP], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_BN], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_BB], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_BR], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_BQ], 0x0000000000000001ull);
  BOOST_CHECK_EQUAL(b->bb[BB_BK], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_BALL], 0x0000000000000001ull);

  BOOST_CHECK_EQUAL(b->bb[BB_WP], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_WN], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_WB], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_WR], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_WQ], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_WK], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_WALL], 0ull);

  BOOST_CHECK_EQUAL(b->bb[BB_EMPTY], 0xfffffffffffffffeull);
  BOOST_CHECK_EQUAL(b->bb[BB_EP], 0ull);
  BOOST_CHECK_EQUAL(b->side, WHITE);
  free(b);

  // En passant capture: c4xb3
  b = fenToBoard("8/8/8/8/1Pp5/8/8/8 b - b3 42 1");
  Move m5 = { PAWN, 26, 17, 0 };
  makeMove(b, m5);

  BOOST_CHECK_EQUAL(b->bb[BB_BP], 0x0000000000020000ull);
  BOOST_CHECK_EQUAL(b->bb[BB_BN], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_BB], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_BR], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_BQ], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_BK], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_BALL], 0x0000000000020000ull);

  BOOST_CHECK_EQUAL(b->bb[BB_WP], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_WN], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_WB], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_WR], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_WQ], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_WK], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_WALL], 0ull);

  BOOST_CHECK_EQUAL(b->bb[BB_EMPTY], 0xfffffffffffdffffull);
  BOOST_CHECK_EQUAL(b->bb[BB_EP], 0ull);
  BOOST_CHECK_EQUAL(b->side, WHITE);
  free(b);
}

BOOST_AUTO_TEST_CASE(testMakeBackwardMove) {
  const char *fen = "3r1q2/2pb2k1/5n2/8/8/2N5/1K2BP2/2Q1R3 w - - 0 0";
  Board *witness = fenToBoard(fen);
  Move m[200];

  Board *b = fenToBoard(fen);
  int numMoves = getAllMoves(b, m, FORWARD);
  for (int i = 0; i < numMoves; i++) {
    makeMove(b, m[i]);
    makeBackwardMove(b, m[i]);
    for (int k = 0; k < BB_COUNT; k++) {
      if (k != BB_EP) {
        BOOST_CHECK_EQUAL(b->bb[k], witness->bb[k]);
      }
    }
  }

  witness->side = BLACK;
  b->side = BLACK;
  numMoves = getAllMoves(b, m, FORWARD);
  for (int i = 0; i < numMoves; i++) {
    makeMove(b, m[i]);
    makeBackwardMove(b, m[i]);
    for (int k = 0; k < BB_COUNT; k++) {
      BOOST_CHECK_EQUAL(b->bb[k], witness->bb[k]);
    }
    BOOST_CHECK_EQUAL(b->side, witness->side);
  }
  free(b);
  free(witness);
}

BOOST_AUTO_TEST_CASE(testMakeMoveSequence) {
  string s1[4] = { "e3", "e6", "b4", "Bxb4"};
  Board *b = makeMoveSequence(4, s1);
  string fen = boardToFen(b);
  BOOST_CHECK_EQUAL(fen.compare("rnbqk1nr/pppp1ppp/4p3/8/1b6/4P3/P1PP1PPP/RNBQKBNR w - - 0 0"), 0);
  free(b);

  string s2[4] = { "e3", "e6", "z4", "Bxb4"};
  b = makeMoveSequence(4, s2);
 BOOST_CHECK(!b);
}

/************************* Tests for movegen.cpp *************************/

BOOST_AUTO_TEST_CASE(testGetWhiteMoves) {
  Board *b;
  Move m[100];
  string s[100];

  b = fenToBoard(NEW_BOARD);
  int numMoves = getAllMoves(b, m, FORWARD);
  getAlgebraicNotation(b, m, numMoves, s);
  const char *expected1[] = { "a3", "b3", "c3", "d3", "e3", "f3", "g3", "h3", "a4", "b4", "c4", "d4", "e4", "f4", "g4", "h4", "Na3", "Nc3", "Nf3", "Nh3" };
  BOOST_CHECK_EQUAL(numMoves, 20);
  BOOST_CHECK_EQUAL_COLLECTIONS(s, s + numMoves, expected1, expected1 + numMoves);
  free(b);

  b = fenToBoard("4q3/8/8/pP3N2/6K1/R5b1/5n1P/4Q3 w - a6 0 1");
  numMoves = getAllMoves(b, m, FORWARD);
  getAlgebraicNotation(b, m, numMoves, s);
  const char *expected2[] = { "hxg3", "bxa6", "Nxg3", "Rxg3", "Rxa5", "Qxe8", "Qxf2", "Qxa5", "Kxg3" };
  BOOST_CHECK_EQUAL(numMoves, 9);
  BOOST_CHECK_EQUAL_COLLECTIONS(s, s + numMoves, expected2, expected2 + numMoves);
  free(b);

  b = fenToBoard("8/4P3/8/8/8/8/q7/8 w - - 0 1");
  numMoves = getAllMoves(b, m, FORWARD);
  getAlgebraicNotation(b, m, numMoves, s);
  const char *expected3[] = { "e8=N", "e8=B", "e8=R", "e8=Q", "e8=K" };
  BOOST_CHECK_EQUAL(numMoves, 5);
  BOOST_CHECK_EQUAL_COLLECTIONS(s, s + numMoves, expected3, expected3 + numMoves);
  free(b);

  // Both the c and e white pawns can capture en passant
  b = fenToBoard("8/8/8/2PpP3/8/8/8/8 w - d6 0 1");
  numMoves = getAllMoves(b, m, FORWARD);
  getAlgebraicNotation(b, m, numMoves, s);
  const char *expected4[] = { "cxd6", "exd6" };
  BOOST_CHECK_EQUAL(numMoves, 2);
  BOOST_CHECK_EQUAL_COLLECTIONS(s, s + numMoves, expected4, expected4 + numMoves);
  free(b);

  // Resolving ambiguities
  b = fenToBoard("8/2q4q/8/7q/1N6/8/1N3N2/6N1 w - - 0 1");
  numMoves = getAllMoves(b, m, FORWARD);
  getAlgebraicNotation(b, m, numMoves, s);
  const char *expected5[] = { "Ne2", "Nf3", "Ngh3", "Nbd1", "Nb2d3", "Na4", "Nc4", "Nfd1", "Nh1", "Nfd3", "Nfh3", "Ne4", "Ng4", "Na2", "Nc2", "N4d3", "Nd5", "Na6", "Nc6" };
  BOOST_CHECK_EQUAL(numMoves, 19);
  BOOST_CHECK_EQUAL_COLLECTIONS(s, s + numMoves, expected5, expected5 + numMoves);
  free(b);

  // Resolving ambiguities
  b = fenToBoard("8/2Q4Q/8/7Q/1n6/8/1n3n2/6n1 w - - 0 1");
  numMoves = getAllMoves(b, m, FORWARD);
  getAlgebraicNotation(b, m, numMoves, s);
  const char *expected6[] = { "Qha5", "Qb5", "Qhc5", "Qd5", "Qhe5", "Q5f5", "Qg5", "Qa7", "Qb7", "Qcd7", "Qce7", "Qcf7", "Qcg7", "Qhd7", "Qhe7", "Qh7f7",
    "Qhg7", "Qh1", "Qhh2", "Qh3", "Qh4", "Q5h6", "Qc1", "Qcc2", "Qc3", "Qc4", "Qcc5", "Qc6", "Qc8", "Q7h6", "Qh8", "Qd1", "Qe2", "Qf3", "Qg4", "Qca5",
    "Qb6", "Qd8", "Qb1", "Qhc2", "Qd3", "Qe4", "Q7f5", "Q7g6", "Q5g6", "Q5f7", "Qe8", "Qch2", "Qg3", "Qf4", "Qce5", "Qd6", "Qb8", "Qg8", };
  BOOST_CHECK_EQUAL(numMoves, 54);
  BOOST_CHECK_EQUAL_COLLECTIONS(s, s + numMoves, expected6, expected6 + numMoves);
  free(b);

  // Diagonal and antidiagonal moves
  b = fenToBoard("8/3Q4/1B6/8/4B1B1/8/3B4/k7 w - - 0 1");
  numMoves = getAllMoves(b, m, FORWARD);
  getAlgebraicNotation(b, m, numMoves, s);
  const char *expected7[] = { "Bc1", "Bde3", "Bf4", "Bg5", "Bh6", "Bb1", "Bc2", "Bd3", "Bef5", "Bg6", "Bh7", "Bd1", "Be2", "Bgf3", "Bh5", "Bba5", "Bc7",
    "Bd8", "Be1", "Bc3", "Bb4", "Bda5", "Bh1", "Bg2", "Bef3", "Bd5", "Bc6", "Bb7", "Ba8", "Bh3", "Bgf5", "Be6", "Bg1", "Bf2", "Bbe3", "Bd4", "Bc5", "Ba7",
    "Qa7", "Qb7", "Qc7", "Qe7", "Qf7", "Qg7", "Qh7", "Qd3", "Qd4", "Qd5", "Qd6", "Qd8", "Qa4", "Qb5", "Qc6", "Qe8", "Qf5", "Qe6", "Qc8" };
  BOOST_CHECK_EQUAL(numMoves, 57);
  BOOST_CHECK_EQUAL_COLLECTIONS(s, s + numMoves, expected7, expected7 + numMoves);
  free(b);
}

BOOST_AUTO_TEST_CASE(testGetBlackMoves) {
  Board *b;
  Move m[100];
  string s[100];

  b = fenToBoard("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR b - - 0 1");
  int numMoves = getAllMoves(b, m, FORWARD);
  getAlgebraicNotation(b, m, numMoves, s);
  const char *expected1[] = { "a6", "b6", "c6", "d6", "e6", "f6", "g6", "h6", "a5", "b5", "c5", "d5", "e5", "f5", "g5", "h5", "Na6", "Nc6", "Nf6", "Nh6" };
  BOOST_CHECK_EQUAL(numMoves, 20);
  BOOST_CHECK_EQUAL_COLLECTIONS(s, s + numMoves, expected1, expected1 + numMoves);
  free(b);

  b = fenToBoard("4q3/5N1p/r5B1/6k1/Pp3n2/8/8/4Q3 b - a3 0 1");
  numMoves = getAllMoves(b, m, FORWARD);
  getAlgebraicNotation(b, m, numMoves, s);
  const char *expected2[] = { "bxa3", "hxg6", "Nxg6", "Rxg6", "Rxa4", "Qxe1", "Qxa4", "Qxf7", "Kxg6" };
  BOOST_CHECK_EQUAL(numMoves, 9);
  BOOST_CHECK_EQUAL_COLLECTIONS(s, s + numMoves, expected2, expected2 + numMoves);
  free(b);

  b = fenToBoard("8/Q7/8/8/8/8/4p3/8 b - - 0 1");
  numMoves = getAllMoves(b, m, FORWARD);
  getAlgebraicNotation(b, m, numMoves, s);
  const char *expected3[] = { "e1=N", "e1=B", "e1=R", "e1=Q", "e1=K" };
  BOOST_CHECK_EQUAL(numMoves, 5);
  BOOST_CHECK_EQUAL_COLLECTIONS(s, s + numMoves, expected3, expected3 + numMoves);
  free(b);

  // Both the c and e white pawns can capture en passant
  b = fenToBoard("8/8/8/8/2pPp3/8/8/8 b - d3 0 1");
  numMoves = getAllMoves(b, m, FORWARD);
  getAlgebraicNotation(b, m, numMoves, s);
  const char *expected4[] = { "cxd3", "exd3" };
  BOOST_CHECK_EQUAL(numMoves, 2);
  BOOST_CHECK_EQUAL_COLLECTIONS(s, s + numMoves, expected4, expected4 + numMoves);
  free(b);

  // Resolving ambiguities
  b = fenToBoard("6n1/1n3n2/8/1n6/7Q/8/2Q4Q/8 b - - 0 1");
  numMoves = getAllMoves(b, m, FORWARD);
  getAlgebraicNotation(b, m, numMoves, s);
  const char *expected5[] = { "Na3", "Nc3", "Nd4", "N5d6", "Na7", "Nc7", "Na5", "Nc5", "Nb7d6", "Nbd8", "Ne5", "Ng5", "Nfd6", "Nfh6", "Nfd8", "Nh8", "Nf6", "Ngh6", "Ne7" };
  BOOST_CHECK_EQUAL(numMoves, 19);
  BOOST_CHECK_EQUAL_COLLECTIONS(s, s + numMoves, expected5, expected5 + numMoves);
  free(b);

  // Resolving ambiguities
  b = fenToBoard("6N1/1N3N2/8/1N6/7q/8/2q4q/8 b - - 0 1");
  numMoves = getAllMoves(b, m, FORWARD);
  getAlgebraicNotation(b, m, numMoves, s);
  const char *expected6[] = { "Qa2", "Qb2", "Qcd2", "Qce2", "Qcf2", "Qcg2", "Qhd2", "Qhe2", "Qh2f2", "Qhg2", "Qha4", "Qb4", "Qhc4", "Qd4", "Qhe4",
    "Q4f4", "Qg4", "Qc1", "Qc3", "Qcc4", "Qc5", "Qc6", "Qcc7", "Qc8", "Qh1", "Q2h3", "Q4h3", "Qh5", "Qh6", "Qhh7", "Qh8", "Qb1", "Qd3", "Qce4", "Qf5",
    "Qg6", "Qch7", "Qg1", "Qe1", "Q4f2", "Q4g3", "Qd1", "Qb3", "Qca4", "Q2g3", "Q2f4", "Qe5", "Qd6", "Qhc7", "Qb8", "Qg5", "Qf6", "Qe7", "Qd8", };
  BOOST_CHECK_EQUAL(numMoves, 54);
  BOOST_CHECK_EQUAL_COLLECTIONS(s, s + numMoves, expected6, expected6 + numMoves);
  free(b);

  // Diagonal and antidiagonal moves
  b = fenToBoard("K7/3b4/8/4b1b1/8/1b6/3q4/8 b - - 0 1");
  numMoves = getAllMoves(b, m, FORWARD);
  getAlgebraicNotation(b, m, numMoves, s);
  const char *expected7[] = { "Ba2", "Bc4", "Bd5", "Bbe6", "Bf7", "Bg8", "Ba1", "Bb2", "Bc3", "Bd4", "Bef6", "Bg7", "Bh8", "Be3", "Bgf4", "Bh6", "Bda4",
    "Bb5", "Bc6", "Be8", "Bd1", "Bc2", "Bba4", "Bh2", "Bg3", "Bef4", "Bd6", "Bc7", "Bb8", "Bh4", "Bgf6", "Be7", "Bd8", "Bh3", "Bg4", "Bf5", "Bde6", "Bc8",
    "Qa2", "Qb2", "Qc2", "Qe2", "Qf2", "Qg2", "Qh2", "Qd1", "Qd3", "Qd4", "Qd5", "Qd6", "Qc1", "Qe3", "Qf4", "Qe1", "Qc3", "Qb4", "Qa5" };
  BOOST_CHECK_EQUAL(numMoves, 57);
  BOOST_CHECK_EQUAL_COLLECTIONS(s, s + numMoves, expected7, expected7 + numMoves);
  free(b);
}

BOOST_AUTO_TEST_CASE(testGetBackwardMoves) {
  Move m[100];
  string s[100];

  Board *b = fenToBoard("8/2Pq3p/6n1/3pB2R/1N1K1P2/1k1Q4/8/4r1b1 b - - 0 0");
  int numMoves = getAllMoves(b, m, BACKWARD);
  getAlgebraicNotation(b, m, numMoves, s);
  const char *expected1[] = { "f3", "c6", "f2", "Na2", "Nc2", "Na6", "Nc6", "Bf6", "Bg7", "Bh8", "Bd6", "Rf5", "Rg5", "Rh1", "Rh2", "Rh3", "Rh4", "Rh6",
    "Qc3", "Qe3", "Qf3", "Qg3", "Qh3", "Qd1", "Qd2", "Qb1", "Qc2", "Qe4", "Qf5", "Qf1", "Qe2", "Qc4", "Qb5", "Qa6", "Kc3", "Ke3", "Kc4", "Ke4", "Kc5" };
  BOOST_CHECK_EQUAL(numMoves, 39);
  BOOST_CHECK_EQUAL_COLLECTIONS(s, s + numMoves, expected1, expected1 + numMoves);

  b->side = WHITE;
  numMoves = getAllMoves(b, m, BACKWARD);
  getAlgebraicNotation(b, m, numMoves, s);
  const char *expected2[] = { "d6", "Nh4", "Ne7", "Nf8", "Nh8", "Bh2", "Bf2", "Be3", "Ra1", "Rb1", "Rc1", "Rd1", "Rf1", "Re2", "Re3", "Re4", "Qe7", "Qf7",
    "Qg7", "Qd6", "Qd8", "Qa4", "Qb5", "Qc6", "Qe8", "Qh3", "Qg4", "Qf5", "Qe6", "Qc8", "Ka2", "Kb2", "Kc2", "Ka3", "Kc3", "Ka4", "Kc4" };
  BOOST_CHECK_EQUAL(numMoves, 37);
  BOOST_CHECK_EQUAL_COLLECTIONS(s, s + numMoves, expected2, expected2 + numMoves);
  free(b);

  // When the EP bit is set, the only legal move that could have been made is the double pawn push
  b = fenToBoard("8/8/8/2p2p2/8/8/P7/8 w - f6 0 0");
  numMoves = getAllMoves(b, m, BACKWARD);
  BOOST_CHECK_EQUAL(numMoves, 1);
  BOOST_CHECK_EQUAL(m[0].piece, PAWN);
  BOOST_CHECK_EQUAL(m[0].from, 37);
  BOOST_CHECK_EQUAL(m[0].to, 53);
  BOOST_CHECK_EQUAL(m[0].promotion, 0);
  free(b);

  b = fenToBoard("8/p7/8/8/2P2P2/8/8/8 b - c3 0 0");
  numMoves = getAllMoves(b, m, BACKWARD);
  BOOST_CHECK_EQUAL(numMoves, 1);
  BOOST_CHECK_EQUAL(m[0].piece, PAWN);
  BOOST_CHECK_EQUAL(m[0].from, 26);
  BOOST_CHECK_EQUAL(m[0].to, 10);
  BOOST_CHECK_EQUAL(m[0].promotion, 0);
  free(b);
}

/************************* Tests for egtb.cpp *************************/

BOOST_AUTO_TEST_CASE(testComboToPieceSets) {
  PieceSet ps[EGTB_MEN];
  int n;

  n = comboToPieceSets("KKKvNP", ps);
  BOOST_CHECK_EQUAL(n, 3);
  BOOST_CHECK_EQUAL(ps[0].side, BLACK);
  BOOST_CHECK_EQUAL(ps[0].piece, PAWN);
  BOOST_CHECK_EQUAL(ps[0].count, 1);
  BOOST_CHECK_EQUAL(ps[1].side, WHITE);
  BOOST_CHECK_EQUAL(ps[1].piece, KING);
  BOOST_CHECK_EQUAL(ps[1].count, 3);
  BOOST_CHECK_EQUAL(ps[2].side, BLACK);
  BOOST_CHECK_EQUAL(ps[2].piece, KNIGHT);
  BOOST_CHECK_EQUAL(ps[2].count, 1);

  n = comboToPieceSets("BBBvQQ", ps);
  BOOST_CHECK_EQUAL(n, 2);
  BOOST_CHECK_EQUAL(ps[0].side, BLACK);
  BOOST_CHECK_EQUAL(ps[0].piece, QUEEN);
  BOOST_CHECK_EQUAL(ps[0].count, 2);
  BOOST_CHECK_EQUAL(ps[1].side, WHITE);
  BOOST_CHECK_EQUAL(ps[1].piece, BISHOP);
  BOOST_CHECK_EQUAL(ps[1].count, 3);

  n = comboToPieceSets("RRPvNP", ps);
  BOOST_CHECK_EQUAL(n, 4);
  BOOST_CHECK_EQUAL(ps[0].side, WHITE);
  BOOST_CHECK_EQUAL(ps[0].piece, PAWN);
  BOOST_CHECK_EQUAL(ps[0].count, 1);
  BOOST_CHECK_EQUAL(ps[1].side, BLACK);
  BOOST_CHECK_EQUAL(ps[1].piece, PAWN);
  BOOST_CHECK_EQUAL(ps[1].count, 1);
  BOOST_CHECK_EQUAL(ps[2].side, WHITE);
  BOOST_CHECK_EQUAL(ps[2].piece, ROOK);
  BOOST_CHECK_EQUAL(ps[2].count, 2);
  BOOST_CHECK_EQUAL(ps[3].side, BLACK);
  BOOST_CHECK_EQUAL(ps[3].piece, KNIGHT);
  BOOST_CHECK_EQUAL(ps[3].count, 1);
}

BOOST_AUTO_TEST_CASE(testEncodeEGTBBoard) {
  PieceSet ps[EGTB_MEN];
  int nps;
  Board *b;

  nps = comboToPieceSets("RBvNN", ps);
  b = fenToBoard("8/3n4/5R2/2B5/8/6n1/8/8 w - - 0 0");
  BOOST_CHECK_EQUAL(encodeEgtbBoard(ps, nps, b), 0b1000101011010101101100110);
  free(b);

  nps = comboToPieceSets("NNvPP", ps);
  b = fenToBoard("2N5/8/4p3/8/5N2/2p5/8/8 b - - 0 0");
  BOOST_CHECK_EQUAL(encodeEgtbBoard(ps, nps, b), 0b0100101011000111011110101);
  free(b);

  nps = comboToPieceSets("NNvPP", ps);
  b = fenToBoard("2N5/8/8/4p3/5N2/2p5/8/8 w - e6 0 0");
  BOOST_CHECK_EQUAL(encodeEgtbBoard(ps, nps, b), 0b0100101111000111011110100);
  free(b);

  nps = comboToPieceSets("PPvP", ps);
  b = fenToBoard("8/8/8/8/2PpP3/8/8/8 b - c3 0 0");
  BOOST_CHECK_EQUAL(encodeEgtbBoard(ps, nps, b), 0b0000100111000110111);
  free(b);
}

BOOST_AUTO_TEST_CASE(testDecodeEgtbBoard) {
  // Tests are complements of the ones in testEncodeEgtbBoard
  PieceSet ps[EGTB_MEN];
  int nps;
  Board *b = (Board*)malloc(sizeof(Board));

  nps = comboToPieceSets("RBvNN", ps);
  decodeEgtbBoard(ps, nps, b, 0b1000101011010101101100110);
  BOOST_CHECK_EQUAL(b->bb[BB_WP], 0x0000000000000000ull);
  BOOST_CHECK_EQUAL(b->bb[BB_WN], 0x0000000000000000ull);
  BOOST_CHECK_EQUAL(b->bb[BB_WB], 0x0000000400000000ull);
  BOOST_CHECK_EQUAL(b->bb[BB_WR], 0x0000200000000000ull);
  BOOST_CHECK_EQUAL(b->bb[BB_WQ], 0x0000000000000000ull);
  BOOST_CHECK_EQUAL(b->bb[BB_WK], 0x0000000000000000ull);
  BOOST_CHECK_EQUAL(b->bb[BB_WALL], 0x0000200400000000ull);

  BOOST_CHECK_EQUAL(b->bb[BB_BP], 0x0000000000000000ull);
  BOOST_CHECK_EQUAL(b->bb[BB_BN], 0x0008000000400000ull);
  BOOST_CHECK_EQUAL(b->bb[BB_BB], 0x0000000000000000ull);
  BOOST_CHECK_EQUAL(b->bb[BB_BR], 0x0000000000000000ull);
  BOOST_CHECK_EQUAL(b->bb[BB_BQ], 0x0000000000000000ull);
  BOOST_CHECK_EQUAL(b->bb[BB_BK], 0x0000000000000000ull);
  BOOST_CHECK_EQUAL(b->bb[BB_BALL], 0x0008000000400000ull);

  BOOST_CHECK_EQUAL(b->bb[BB_EMPTY], 0xfff7dffbffbfffffull);
  BOOST_CHECK_EQUAL(b->bb[BB_EP], 0ull);
  BOOST_CHECK_EQUAL(b->side, WHITE);

  nps = comboToPieceSets("NNvPP", ps);
  decodeEgtbBoard(ps, nps, b, 0b0100101011000111011110101);
  BOOST_CHECK_EQUAL(b->bb[BB_WP], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_WN], 0x0400000020000000ull);
  BOOST_CHECK_EQUAL(b->bb[BB_WB], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_WR], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_WQ], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_WK], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_WALL], 0x0400000020000000ull);

  BOOST_CHECK_EQUAL(b->bb[BB_BP], 0x0000100000040000ull);
  BOOST_CHECK_EQUAL(b->bb[BB_BN], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_BB], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_BR], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_BQ], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_BK], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_BALL], 0x0000100000040000ull);

  BOOST_CHECK_EQUAL(b->bb[BB_EMPTY], 0xfbffefffdffbffffull);
  BOOST_CHECK_EQUAL(b->bb[BB_EP], 0ull);
  BOOST_CHECK_EQUAL(b->side, BLACK);

  nps = comboToPieceSets("NNvPP", ps);
  decodeEgtbBoard(ps, nps, b, 0b0100101111000111011110100);
  BOOST_CHECK_EQUAL(b->bb[BB_WP], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_WN], 0x0400000020000000ull);
  BOOST_CHECK_EQUAL(b->bb[BB_WB], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_WR], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_WQ], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_WK], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_WALL], 0x0400000020000000ull);

  BOOST_CHECK_EQUAL(b->bb[BB_BP], 0x0000001000040000ull);
  BOOST_CHECK_EQUAL(b->bb[BB_BN], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_BB], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_BR], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_BQ], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_BK], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_BALL], 0x0000001000040000ull);

  BOOST_CHECK_EQUAL(b->bb[BB_EMPTY], 0xfbffffefdffbffffull);
  BOOST_CHECK_EQUAL(b->bb[BB_EP], 0x0000100000000000ull);
  BOOST_CHECK_EQUAL(b->side, WHITE);

  nps = comboToPieceSets("PPvP", ps);
  decodeEgtbBoard(ps, nps, b, 0b0000100111000110111);
  BOOST_CHECK_EQUAL(b->bb[BB_WP], 0x0000000014000000ull);
  BOOST_CHECK_EQUAL(b->bb[BB_WN], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_WB], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_WR], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_WQ], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_WK], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_WALL], 0x0000000014000000ull);

  BOOST_CHECK_EQUAL(b->bb[BB_BP], 0x0000000008000000ull);
  BOOST_CHECK_EQUAL(b->bb[BB_BN], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_BB], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_BR], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_BQ], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_BK], 0ull);
  BOOST_CHECK_EQUAL(b->bb[BB_BALL], 0x0000000008000000ull);

  BOOST_CHECK_EQUAL(b->bb[BB_EMPTY], 0xffffffffe3ffffffull);
  BOOST_CHECK_EQUAL(b->bb[BB_EP], 0x0000000000040000ull);
  BOOST_CHECK_EQUAL(b->side, BLACK);
  free(b);
}

BOOST_AUTO_TEST_CASE(testGetEgtbSize) {
  PieceSet ps[EGTB_MEN];
  int numPieceSets = comboToPieceSets("KRvK", ps);
  BOOST_CHECK_EQUAL(numPieceSets, 3);
  BOOST_CHECK_EQUAL(getEgtbSize(ps, numPieceSets), 78120);

  numPieceSets = comboToPieceSets("KPPvNP", ps);
  BOOST_CHECK_EQUAL(numPieceSets, 4);
  BOOST_CHECK_EQUAL(getEgtbSize(ps, numPieceSets), 193950720);

  numPieceSets = comboToPieceSets("KPPvN", ps);
  BOOST_CHECK_EQUAL(numPieceSets, 3);
  BOOST_CHECK_EQUAL(getEgtbSize(ps, numPieceSets), 4356864);

  numPieceSets = comboToPieceSets("NNPvPP", ps);
  BOOST_CHECK_EQUAL(numPieceSets, 3);
  BOOST_CHECK_EQUAL(getEgtbSize(ps, numPieceSets), 94955040);
  BOOST_CHECK_EQUAL(getEpEgtbSize(ps, numPieceSets), 1053976);

  numPieceSets = comboToPieceSets("NPPvPP", ps);
  BOOST_CHECK_EQUAL(numPieceSets, 3);
  BOOST_CHECK_EQUAL(getEpEgtbSize(ps, numPieceSets), 1536304);

  numPieceSets = comboToPieceSets("PPPvPP", ps);
  BOOST_CHECK_EQUAL(numPieceSets, 2);
  BOOST_CHECK_EQUAL(getEpEgtbSize(ps, numPieceSets), 556248);
}

BOOST_AUTO_TEST_CASE(testGetEgtbIndex) {
  Board *b;
  PieceSet ps[EGTB_MEN];
  int nps;

  nps = comboToPieceSets("KvR", ps);
  b = fenToBoard("8/8/8/8/8/8/8/Kr6 w - - 0 0");
  BOOST_CHECK_EQUAL(getEgtbIndex(ps, nps, b), 0);
  free(b);
  b = fenToBoard("r7/8/8/8/8/8/8/K7 w - - 0 0");
  BOOST_CHECK_EQUAL(getEgtbIndex(ps, nps, b), 110);
  free(b);
  b = fenToBoard("r7/8/8/8/8/8/8/K7 b - - 0 0");
  BOOST_CHECK_EQUAL(getEgtbIndex(ps, nps, b), 111);
  free(b);
  b = fenToBoard("r7/8/8/8/3K4/8/8/8 w - - 0 0");
  BOOST_CHECK_EQUAL(getEgtbIndex(ps, nps, b), 1244);
  free(b);

  nps = comboToPieceSets("QPPvNP", ps);
  b = fenToBoard("8/8/8/5p2/2n5/P7/3P4/5Q2 b - - 0 0");
  BOOST_CHECK_EQUAL(getEgtbIndex(ps, nps, b), 6595967);
  free(b);

  // En passant cases
  nps = comboToPieceSets("NNPvPP", ps);
  b = fenToBoard("8/8/N4N2/1pP5/8/5p2/8/8 w - b6 0 0");
  BOOST_CHECK_EQUAL(getEgtbIndex(ps, nps, b), 95128708);
  free(b);
  b = fenToBoard("8/5N2/8/5p2/NpP5/8/8/8 b - c3 0 0");
  BOOST_CHECK_EQUAL(getEgtbIndex(ps, nps, b), 95751805);
  free(b);
}

/************************* Tests for fileutil.cpp *************************/

BOOST_AUTO_TEST_CASE(testGetFileSize) {
  FILE *f = fopen("/tmp/foo.dat", "w");
  fprintf(f, "abcdefghij");
  fclose(f);
  BOOST_CHECK_EQUAL(getFileSize("/tmp/foo.dat"), 10);

  f = fopen("/tmp/foo.dat", "w");
  fclose(f);
  BOOST_CHECK_EQUAL(getFileSize("/tmp/foo.dat"), 0);
}

/************************* Tests for lruCache.cpp *************************/

BOOST_AUTO_TEST_CASE(testLruCache) {
  char *x = (char*)malloc(3);
  char *y = (char*)malloc(3);
  char *z = (char*)malloc(3);
  LruCache cache = lruCacheCreate(2);
  lruCachePut(&cache, 17, x);
  BOOST_CHECK_EQUAL(lruCacheGet(&cache, 17), x);
  lruCachePut(&cache, 103, y);

  // Now x is the LRU and should be evicted when we insert one more element
  lruCachePut(&cache, 294, z);
  BOOST_CHECK_EQUAL(lruCacheGet(&cache, 103), y);
  BOOST_CHECK_EQUAL(lruCacheGet(&cache, 294), z);
  BOOST_CHECK(!lruCacheGet(&cache, 17));
  BOOST_CHECK_EQUAL(cache.lookups, 4);
  BOOST_CHECK_EQUAL(cache.misses, 1);
  BOOST_CHECK_EQUAL(cache.evictions, 1);
}

/************************* Tests for stringutil.cpp *************************/

BOOST_AUTO_TEST_CASE(testEndsWith) {
  BOOST_CHECK(endsWith("mama", "a"));
  BOOST_CHECK(endsWith("mama", "mama"));
  BOOST_CHECK(endsWith("mama", ""));
  BOOST_CHECK(!endsWith("mama", "b"));
  BOOST_CHECK(!endsWith("", "a"));
}

BOOST_AUTO_TEST_CASE(testTrim) {
  const char s1[5] = "mama";
  BOOST_CHECK_EQUAL(trim((char*)s1), "mama");

  const char s2[15] = "   mama\t\t \n  ";
  BOOST_CHECK_EQUAL(trim((char*)s2), "mama");

  const char s3[1] = "";
  BOOST_CHECK_EQUAL(trim((char*)s3), "");
}

BOOST_AUTO_TEST_CASE(testSplit) {
  const char s1[8] = "foo=bar";
  BOOST_CHECK_EQUAL(split((char*)s1, '='), "bar");
  BOOST_CHECK_EQUAL(s1, "foo");

  const char s2[8] = "foo0bar";
  BOOST_CHECK(!split((char*)s2, '='));
  BOOST_CHECK_EQUAL(s2, "foo0bar");
}

BOOST_AUTO_TEST_CASE(testUnquote) {
  const char s1[5] = "mama";
  BOOST_CHECK_EQUAL(unquote((char*)s1), "mama");

  const char s2[7] = "\"mama\"";
  BOOST_CHECK_EQUAL(unquote((char*)s2), "mama");

  const char s3[6] = "\"mama";
  BOOST_CHECK_EQUAL(unquote((char*)s3), "\"mama");

  const char s4[2] = "m";
  BOOST_CHECK_EQUAL(unquote((char*)s4), "m");
}

BOOST_AUTO_TEST_CASE(testIsFen) {
  BOOST_CHECK_EQUAL(isFen("8/8/8/4N3/8/3b4/8/8 w - - 42 1"), true);
  BOOST_CHECK_EQUAL(isFen("8/8/8/4N3/8/3b4/8 w - - 42 1"), false); // describes only 7 lines, not 8
  BOOST_CHECK_EQUAL(isFen("e3 e6 b4 Bxb4"), false);
  BOOST_CHECK_EQUAL(isFen("mama tata"), false);
}
