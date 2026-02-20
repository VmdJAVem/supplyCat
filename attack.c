#define _POSIX_C_SOURCE 199309L
#include "attack.h"
#include <stdlib.h>

// Attack tables definition
bitboard knightAttacks[64];
bitboard kingAttacks[64];
bitboard pawnAttacks[2][64];

int rookOffsets[4] = {8, -8, -1, 1};
int bishopOffsets[4] = {-9, -7, 7, 9};
int queenOffsets[8] = {8, -8, -1, 1, -9, -7, 7, 9};

void initAttackTables(void) {
	for (int i = 0; i < 64; i++) {
		knightAttacks[i] = computeKnightAttacks(i);
		kingAttacks[i] = computeKingAttacks(i);
		pawnAttacks[blancas][i] = computePawnAttacks(blancas, i);
		pawnAttacks[negras][i] = computePawnAttacks(negras, i);
	}
}

bitboard computeKnightAttacks(casilla sq) {
	int rank = sq / 8;
	int file = sq % 8;
	bitboard result = 0;

	int moves[8][2] = {
		{2,1}, {2,-1}, {-2,1}, {-2,-1},
		{1,2}, {1,-2}, {-1,2}, {-1,-2}
	};

	for (int i = 0; i < 8; i++) {
		int newRank = rank + moves[i][0];
		int newFile = file + moves[i][1];

		if (newRank >= 0 && newRank < 8 && newFile >= 0 && newFile < 8) {
			int destSquare = newRank * 8 + newFile;
			result |= BB_SQUARE(destSquare);
		}
	}
	return result;
}

bitboard computeKingAttacks(casilla sq) {
	int rank = sq / 8;
	int file = sq % 8;
	bitboard result = 0;

	int moves[8][2] = {
		{1,0}, {1,1}, {0,1}, {-1,1},
		{-1,0}, {-1,-1}, {0,-1}, {1,-1}
	};

	for (int i = 0; i < 8; i++) {
		int newRank = rank + moves[i][0];
		int newFile = file + moves[i][1];
		if (newRank >= 0 && newRank < 8 && newFile >= 0 && newFile < 8) {
			int destSquare = newRank * 8 + newFile;
			result |= BB_SQUARE(destSquare);
		}
	}
	return result;
}

bitboard computePawnAttacks(color c, casilla sq) {
	int rank = sq / 8;
	int file = sq % 8;
	bitboard result = 0;

	int moves[2][2][2] = {
		{{1,-1}, {1,1}},
		{{-1,-1}, {-1,1}}
	};
	int color = (c == blancas) ? 0 : 1;

	for (int i = 0; i < 2; i++) {
		int newRank = rank + moves[color][i][0];
		int newFile = file + moves[color][i][1];

		if (newRank >= 0 && newRank < 8 && newFile >= 0 && newFile < 8) {
			int destSquare = newRank * 8 + newFile;
			result |= BB_SQUARE(destSquare);
		}
	}
	return result;
}
