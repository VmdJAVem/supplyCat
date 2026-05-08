#include "include/bitboard.h"

bitboard knightAttacks[64];
bitboard kingAttacks[64];
bitboard pawnAttacks[2][64];
bitboard rookMask[64][4];
bitboard bishopMask[64][4];

void printBitboard(bitboard bb) {
	printf("\n");
	for (int rank = 7; rank >= 0; rank--) {
		printf("%d ", rank + 1);
		for (int file = 0; file < 8; file++) {
			int square = rank * 8 + file;
			printf("%c ", (bb >> square) & 1 ? 'x' : '.');
		}
		printf("\n");
	}
	printf("  a b c d e f g h\n\n");
}

bool isAttacked(Tablero * t, int square, color attackerColor) {
	if (kingAttacks[square] & t->piezas[attackerColor][rey]) {
		return true;
	}
	if (knightAttacks[square] & t->piezas[attackerColor][caballo]) {
		return true;
	}
	if (pawnAttacks[!attackerColor][square] & t->piezas[attackerColor][peon]) {
		return true;
	}
	for (int d = 0; d < 4; d++) {
		bitboard ray = rookMask[square][d];
		bitboard blockers = ray & t->allOccupiedSquares;
		if (blockers) {
			int firstBlocker;
			if (d < 2)
				firstBlocker = __builtin_ctzll(blockers);
			else
				firstBlocker = 63 - __builtin_clzll(blockers);

			if ((BB_SQUARE(firstBlocker) &
			     (t->piezas[attackerColor][torre] | t->piezas[attackerColor][reina])))
				return true;
		}
	}

	for (int d = 0; d < 4; d++) {
		bitboard ray = bishopMask[square][d];
		bitboard blockers = ray & t->allOccupiedSquares;
		if (blockers) {
			int firstBlocker;
			if (d > 1)
				firstBlocker = __builtin_ctzll(blockers);
			else
				firstBlocker = 63 - __builtin_clzll(blockers);

			if ((BB_SQUARE(firstBlocker) &
			     (t->piezas[attackerColor][alfil] | t->piezas[attackerColor][reina])))
				return true;
		}
	}

	return false;
}

bitboard computeKnightAttacks(casilla sq) {
	int rank = sq / 8;
	int file = sq % 8;
	bitboard result = 0;

	int moves[8][2] = {{2, 1}, {2, -1}, {-2, 1}, {-2, -1}, {1, 2}, {1, -2}, {-1, 2}, {-1, -2}};
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

	int moves[8][2] = {{1, 0}, {1, 1}, {0, 1}, {-1, 1}, {-1, 0}, {-1, -1}, {0, -1}, {1, -1}};
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
	int moves[2][2][2] = {{{1, -1}, {1, 1}}, {{-1, -1}, {-1, 1}}};
	int colorIdx = (c == blancas) ? 0 : 1;

	for (int i = 0; i < 2; i++) {
		int newRank = rank + moves[colorIdx][i][0];
		int newFile = file + moves[colorIdx][i][1];

		if (newRank >= 0 && newRank < 8 && newFile >= 0 && newFile < 8) {
			int destSquare = newRank * 8 + newFile;
			result |= BB_SQUARE(destSquare);
		}
	}

	return result;
}

void initAttackTables() {
	for (int i = 0; i < 64; i++) {
		knightAttacks[i] = computeKnightAttacks(i);
	}
	for (int i = 0; i < 64; i++) {
		kingAttacks[i] = computeKingAttacks(i);
	}
	for (int i = 0; i < 64; i++) {
		pawnAttacks[blancas][i] = computePawnAttacks(blancas, i);
		pawnAttacks[negras][i] = computePawnAttacks(negras, i);
	}
	for (int i = 0; i < 64; i++) {
		bitboard northRay = 0;
		int to = i + 8;
		while (to < 64) {
			northRay |= BB_SQUARE(to);
			to += 8;
		}
		rookMask[i][0] = northRay;

		bitboard eastRay = 0;
		to = i + 1;
		while (to < 64 && (to / 8) == (i / 8)) {
			eastRay |= BB_SQUARE(to);
			to++;
		}
		rookMask[i][1] = eastRay;

		bitboard westRay = 0;
		to = i - 1;
		while (to >= 0 && (to / 8) == (i / 8)) {
			westRay |= BB_SQUARE(to);
			to--;
		}
		rookMask[i][2] = westRay;

		bitboard southRay = 0;
		to = i - 8;
		while (to >= 0) {
			southRay |= BB_SQUARE(to);
			to -= 8;
		}
		rookMask[i][3] = southRay;
	}

	for (int i = 0; i < 64; i++) {
		bitboard upLeftRay = 0;
		int to = i - 9;
		while (to >= 0 && to < 64 && abs((to % 8) - (i % 8)) == abs((to / 8) - (i / 8))) {
			upLeftRay |= BB_SQUARE(to);
			to -= 9;
		}
		bishopMask[i][0] = upLeftRay;

		bitboard upRightRay = 0;
		to = i - 7;
		while (to >= 0 && to < 64 && abs((to % 8) - (i % 8)) == abs((to / 8) - (i / 8))) {
			upRightRay |= BB_SQUARE(to);
			to -= 7;
		}
		bishopMask[i][1] = upRightRay;

		bitboard downLeftRay = 0;
		to = i + 7;
		while (to >= 0 && to < 64 && abs((to % 8) - (i % 8)) == abs((to / 8) - (i / 8))) {
			downLeftRay |= BB_SQUARE(to);
			to += 7;
		}
		bishopMask[i][2] = downLeftRay;

		bitboard downRightRay = 0;
		to = i + 9;
		while (to >= 0 && to < 64 && abs((to % 8) - (i % 8)) == abs((to / 8) - (i / 8))) {
			downRightRay |= BB_SQUARE(to);
			to += 9;
		}
		bishopMask[i][3] = downRightRay;
	}
}

bitboard attackedByColor(Tablero * t, color attacker) {
	bitboard attacked = 0;

	bitboard knights = t->piezas[attacker][caballo];
	while (knights) {
		int sq = __builtin_ctzll(knights);
		knights &= knights - 1;
		attacked |= knightAttacks[sq];
	}

	bitboard kings = t->piezas[attacker][rey];
	while (kings) {
		int sq = __builtin_ctzll(kings);
		kings &= kings - 1;
		attacked |= kingAttacks[sq];
	}

	bitboard pawns = t->piezas[attacker][peon];
	while (pawns) {
		int sq = __builtin_ctzll(pawns);
		pawns &= pawns - 1;
		attacked |= pawnAttacks[attacker][sq];
	}

	bitboard orthPieces = t->piezas[attacker][torre] | t->piezas[attacker][reina];
	while (orthPieces) {
		int sq = __builtin_ctzll(orthPieces);
		orthPieces &= orthPieces - 1;
		for (int d = 0; d < 4; d++) {
			bitboard ray = rookMask[sq][d];
			bitboard blockers = ray & t->allOccupiedSquares;
			if (blockers) {
				int firstBlocker;
				if (d < 2)
					firstBlocker = __builtin_ctzll(blockers);
				else
					firstBlocker = 63 - __builtin_clzll(blockers);

				bitboard before;
				if (d < 2)
					before = ray & ((1ULL << firstBlocker) - 1);
				else
					before = ray & ~((1ULL << (firstBlocker + 1)) - 1);

				attacked |= before;

				if (BB_SQUARE(firstBlocker) & t->allPieces[!attacker]) {
					attacked |= BB_SQUARE(firstBlocker);
				}
			} else {
				attacked |= ray;
			}
		}
	}

	bitboard diaPieces = t->piezas[attacker][alfil] | t->piezas[attacker][reina];
	while (diaPieces) {
		int sq = __builtin_ctzll(diaPieces);
		diaPieces &= diaPieces - 1;
		for (int d = 0; d < 4; d++) {
			bitboard ray = bishopMask[sq][d];
			bitboard blockers = ray & t->allOccupiedSquares;
			if (blockers) {
				int firstBlocker;
				if (d > 1)
					firstBlocker = __builtin_ctzll(blockers);
				else
					firstBlocker = 63 - __builtin_clzll(blockers);

				bitboard before;
				if (d > 1)
					before = ray & ((1ULL << firstBlocker) - 1);
				else
					before = ray & ~((1ULL << (firstBlocker + 1)) - 1);

				attacked |= before;

				if (BB_SQUARE(firstBlocker) & t->allPieces[!attacker]) {
					attacked |= BB_SQUARE(firstBlocker);
				}
			} else {
				attacked |= ray;
			}
		}
	}

	return attacked;
}

bitboard BB_FILE(int file) { return (C64(0x0101010101010101) << file); }