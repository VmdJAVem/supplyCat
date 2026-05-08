#ifndef BITBOARD_H
#define BITBOARD_H

#include "types.h"

void initAttackTables();
bitboard computeKnightAttacks(casilla sq);
bitboard computeKingAttacks(casilla sq);
bitboard computePawnAttacks(color c, casilla sq);
void printBitboard(bitboard bb);
bitboard attackedByColor(Tablero * t, color attacker);
bool isAttacked(Tablero * t, int square, color attackerColor);
bitboard BB_FILE(int file);

#endif