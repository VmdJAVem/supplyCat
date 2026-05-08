#ifndef SEARCH_H
#define SEARCH_H

#include "types.h"
#include "bitboard.h"
#include "uci.h"

float recursiveNegaMax(int depth, Tablero * t, color c, float alpha, float beta);
moveScore negaMax(Tablero * t, color c, int timeLimit);
moveScore negaMaxFixedDepth(Tablero * t, color c, int depth);
float quiescence(Tablero * t, color c, float alpha, float beta, int qdepth);

moveSort scoreMoveForSorting(Move * move, int depth);
void moveToMoveSort(moveLists * input, moveSort output[], int depth);
int compareMoveSort(const void * a, const void * b);
void insertionSort(moveSort * moves, int count);
bool isEqualMoves(Move * x, Move * y);

void testZobrist();
void testTT();
void testNPS();
bool testUnmakeMove(Tablero * original, Move move, color c);
void testUnmakeAll();

#endif