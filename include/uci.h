#ifndef UCI_H
#define UCI_H

#include "types.h"

bool inputAvaliable();
void proccesUCICommands(char command[4096], Tablero * t);

#endif