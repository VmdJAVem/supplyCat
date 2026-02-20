#ifndef BOARD_H
#define BOARD_H

#include "types.h"

// Castling rights constants
extern const uint8_t WHITE_OO;
extern const uint8_t WHITE_OOO;
extern const uint8_t BLACK_OO;
extern const uint8_t BLACK_OOO;

// Function declarations
void initBoard(Tablero *t);
void updateBoardCache(Tablero *t);
void printBitboard(bitboard bb);
bool isAttacked(Tablero *t, int square, color attackerColor);
float boardEval(Tablero *t, color c);

#endif // BOARD_H
