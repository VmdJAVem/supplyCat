#ifndef ATTACK_H
#define ATTACK_H

#include "types.h"

// Attack tables
extern bitboard knightAttacks[64];
extern bitboard kingAttacks[64];
extern bitboard pawnAttacks[2][64];

// Sliding pieces offsets
extern int rookOffsets[4];
extern int bishopOffsets[4];
extern int queenOffsets[8];

// Function declarations
void initAttackTables(void);
bitboard computeKnightAttacks(casilla sq);
bitboard computeKingAttacks(casilla sq);
bitboard computePawnAttacks(color c, casilla sq);

#endif // ATTACK_H
