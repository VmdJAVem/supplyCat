#ifndef UTILS_H
#define UTILS_H

#include "types.h"

tipoDePieza charToPiece(char c);
casilla stringToSq(const char * sq);
char pieceToChar(tipoDePieza piece);
char * moveToStr(Move * move);

#endif