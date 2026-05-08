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

	testZobrist();
	testTT();
	testNPS();
	testUnmakeAll();

	return 0;
}