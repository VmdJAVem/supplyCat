#include "include/types.h"
#include "include/bitboard.h"
#include "include/board.h"
#include "include/movegen.h"
#include "include/search.h"
#include "include/eval.h"
#include "include/uci.h"

int main() {
	setbuf(stdout, NULL);
	setbuf(stdin, NULL);

	initAttackTables();
	initZobrist();

	while (true) {
		if (inputAvaliable()) {
			char buffer[4096];
			if (fgets(buffer, sizeof(buffer), stdin)) {
				proccesUCICommands(buffer, &tablero);
			}
		} else {
			struct timespec ts = {0, 1000000};
			nanosleep(&ts, NULL);
		}
	}
	/*
	testZobrist();
	testTT();
	testNPS();
	testUnmakeAll();
*/
	return 0;
}
