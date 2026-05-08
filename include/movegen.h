#ifndef MOVEGEN_H
#define MOVEGEN_H

#include "types.h"

void generateAllMoves(color c, Tablero * t, moveLists * output);
void generateAllPseudoMoves(color c, Tablero * t, moveLists * output);
void generateLegalMoves(color c, Tablero * t, moveLists * output);
void generateCaptures(color c, Tablero * t, moveLists * output);

void generateKnightMoves(moveLists * ml, color c, Tablero * t);
void generateKingMoves(moveLists * ml, color c, Tablero * t);
void generatePawnMoves(moveLists * ml, color c, Tablero * t);
void generateRookMoves(moveLists * ml, color c, Tablero * t);
void generateBishopMoves(moveLists * ml, color c, Tablero * t);
void generateQueenMoves(moveLists * ml, color c, Tablero * t);

void generateKnightCaptures(moveLists * ml, color c, Tablero * t);
void generateKingCaptures(moveLists * ml, color c, Tablero * t);
void generatePawnCaptures(moveLists * ml, color c, Tablero * t);
void generateRookCaptures(moveLists * ml, color c, Tablero * t);
void generateBishopCaptures(moveLists * ml, color c, Tablero * t);
void generateQueenCaptures(moveLists * ml, color c, Tablero * t);

#endif