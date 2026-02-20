#ifndef SEARCH_H
#define SEARCH_H

#include "types.h"

// Function declarations
moveScore negaMax(Tablero *t, color c, int timeLimit);
moveScore negaMaxFixedDepth(Tablero *t, color c, int depth);
float recursiveNegaMax(int depth, Tablero *t, color c, float alpha, float beta);

#endif // SEARCH_H
