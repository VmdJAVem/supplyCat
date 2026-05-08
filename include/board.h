#ifndef BOARD_H
#define BOARD_H

#include "types.h"

void initBoard(Tablero * t);
void updateBoardCache(Tablero * t);
void makeMove(Move * move, Tablero * t, color c);
void unmakeMove(Move * move, Tablero * t, color c, int oldEnPassant, uint8_t oldCastling, int oldHalfClock, int oldFullClock);
void initZobrist();
uint64_t computeZobrist(Zobrist * z, Tablero * t, color sideToMove);
bool isRepetition(uint64_t hash);

tipoDePieza charToPiece(char c);
casilla stringToSq(const char * sq);
char pieceToChar(tipoDePieza piece);
char * moveToStr(Move * move);

#endif