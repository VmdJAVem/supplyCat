#ifndef MOVEGEN_H
#define MOVEGEN_H

#include "types.h"

// Function declarations
void generateAllMoves(color c, Tablero *t, moveLists *output);
void generateKnightMoves(moveLists *ml, color c, Tablero *t);
void generateKingMoves(moveLists *ml, color c, Tablero *t);
void generatePawnMoves(moveLists *ml, color c, Tablero *t);
void generateRookMoves(moveLists *ml, color c, Tablero *t);
void generateBishopMoves(moveLists *ml, color c, Tablero *t);
void generateQueenMoves(moveLists *ml, color c, Tablero *t);

#endif // MOVEGEN_H
