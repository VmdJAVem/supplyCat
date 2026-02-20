#define _POSIX_C_SOURCE 199309L
#include "types.h"
#include "attack.h"
#include "board.h"
#include "movegen.h"
#include "move.h"
#include "search.h"
#include "uci.h"
#include <stdio.h>
#include <string.h>

int main() {
	Tablero tablero = {0};
	initAttackTables();

	while (true) {
		if (inputAvaliable()) {
			char buffer[256];
			if (fgets(buffer, sizeof(buffer), stdin)) {
				proccesUCICommands(buffer, &tablero);
			}
		}
	}

	return 0;
}
