#ifndef MOVE_H
#define MOVE_H

#include "types.h"

// Function declarations
void makeMove(Move *move, Tablero *t, color c);
void unmakeMove(Move *move, Tablero *t, color c);
tipoDePieza charToPiece(char c);
casilla stringToSq(const char *sq);
char pieceToChar(tipoDePieza piece);
char *moveToStr(Move *move);

#endif // MOVE_H
