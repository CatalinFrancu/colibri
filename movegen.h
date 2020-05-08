#ifndef __MOVEGEN_H__
#define __MOVEGEN_H__
#include "defines.h"

/*
 * Get all legal moves on the given board.
 *
 * When direction = BACKWARD, this function returns all backward moves, under
 * some assumptions:
 *
 * 1. No uncaptures and no unpromotions (i.e., the piece set remains unchanged).
 *
 * 2. En passant is handled with the EGTB in mind. If the EP bit is set, the
 *    only legal unmove is a two-step unmove of the corresponding pawn. This
 *    is correct. If the EP bit is not set, in theory there should not be any
 *    two-step unmoves. However, for space conservation, our EGTB only encode
 *    EP positions where an EP capture is actually possible (i.e. there is an
 *    opposing pawn immediately to the left or right). So we forbid double
 *    pawn unmoves when there is an opposing pawn nearby, but allow them
 *    otherwise.
 *
 * 3. When we index en passant positions in the EGTB, for ease of
 *    implementation we don't ensure that the ep square (say, c6) and the one
 *    behind it (c7) are free. This does not to cause any verification
 *    failures.
 *
 * 4. Some unmoves are illegal. For example, in Na1/ke1/b, the function will
 *    return the unmoves Nb3 and Nc2. In reality, Nc2 is illegal because from
 *    the parent position Nc2/ke1/w, White's only legal move would have been
 *    Nxe1. This does not cause a problem during EGTB generation because the
 *    parent position allows captures and therefore is already evaluated and
 *    doesn't care that we notify it of a wrong child.
 */
int getAllMoves(Board *b, Move *m, bool direction);

#endif
