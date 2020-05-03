#ifndef __MOVEGEN_H__
#define __MOVEGEN_H__
#include "defines.h"

/* Get all legal moves on the given board.
 * When direction = BACKWARD, this function returns all backward moves, under some assumptions:
 * - no un-captures and no un-promotions (i.e., the piece set remains unchanged)
 * - ignores the en passant field. (1)
 * - some of these moves may be illegal and should be checked with the forward move generator. (2)
 *
 * (1) So if the en passant field is e3, this routine doesn't realize that the only legal backward move is e4.
 * (2) For example, with White Na1 and Black Ke1, this function will produce the backward moves Nb3 and Nc2. In reality, Nc2 is illegal because,
 * if the original position had been Nc2 / Ke1, then White's only legal move would have been Nxe1.
 * (2) When we index en passant positions in the EGTB, for the ease of implementation, we don't ensure that the ep square (say, c6) and the
 * one behind it (c7) are free. The forward move generator should make sure of that.
 */
int getAllMoves(Board *b, Move *m, bool direction);

#endif
