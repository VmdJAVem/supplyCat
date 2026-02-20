#define _POSIX_C_SOURCE 199309L
#include "board.h"
#include "attack.h"
#include "movegen.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

const uint8_t WHITE_OO = 1;
const uint8_t WHITE_OOO = 2;
const uint8_t BLACK_OO = 4;
const uint8_t BLACK_OOO = 8;

void initBoard(Tablero *t) {
	if (debug) {
		printf("DEBUG: initBoard called\n");
	}
	memset(t, 0, sizeof(Tablero));

	// blancas
	t->piezas[blancas][peon] = RANK(2 - 1);
	t->piezas[blancas][caballo] = BB_SQUARE(b1) | BB_SQUARE(g1);
	t->piezas[blancas][alfil] = BB_SQUARE(c1) | BB_SQUARE(f1);
	t->piezas[blancas][torre] = BB_SQUARE(a1) | BB_SQUARE(h1);
	t->piezas[blancas][reina] = BB_SQUARE(d1);
	t->piezas[blancas][rey] = BB_SQUARE(e1);

	// negras
	t->piezas[negras][peon] = RANK(7 - 1);
	t->piezas[negras][caballo] = BB_SQUARE(b8) | BB_SQUARE(g8);
	t->piezas[negras][alfil] = BB_SQUARE(c8) | BB_SQUARE(f8);
	t->piezas[negras][torre] = BB_SQUARE(a8) | BB_SQUARE(h8);
	t->piezas[negras][reina] = BB_SQUARE(d8);
	t->piezas[negras][rey] = BB_SQUARE(e8);

	t->castlingRights = WHITE_OO | WHITE_OOO | BLACK_OO | BLACK_OOO;
	updateBoardCache(t);
}

void updateBoardCache(Tablero *t) {
	t->allPieces[blancas] = t->piezas[blancas][peon] |
		t->piezas[blancas][caballo] |
		t->piezas[blancas][alfil] |
		t->piezas[blancas][torre] |
		t->piezas[blancas][reina] |
		t->piezas[blancas][rey];

	t->allPieces[negras] = t->piezas[negras][peon] |
		t->piezas[negras][caballo] |
		t->piezas[negras][alfil] |
		t->piezas[negras][torre] |
		t->piezas[negras][reina] |
		t->piezas[negras][rey];

	t->allOccupiedSquares = t->allPieces[blancas] | t->allPieces[negras];
}

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
bool isAttacked(Tablero *t, int square, color attackerColor){
	bitboard king = t->piezas[attackerColor][rey];
	if(king){
		int from = __builtin_ctzll(king);
		bitboard sqAttackedByKing = kingAttacks[from];
		if (sqAttackedByKing & (1ULL << square)) return true;
	}
}
float boardEval(Tablero *t, color c) {
	if (t->piezas[c][rey] == 0) {
		return 0;
	}

	casilla king = __builtin_ctzll(t->piezas[c][rey]);
	moveLists movePerColor[2] = {0};
	moveLists possibleMoves = {0};
	generateAllMoves(c, t, &possibleMoves);

	if (possibleMoves.count == 0) {
		if (isAttacked(t, king, !c)) {
			return -1.0f;
		} else if (possibleMoves.count == 0) {
			return 0.0f;
		}
	}

	movePerColor[c] = possibleMoves;
	generateAllMoves(!c, t, &movePerColor[!c]);

	float value =
		1 * (__builtin_popcountll(t->piezas[c][peon]) - __builtin_popcountll(t->piezas[!c][peon])) +
		3 * ((__builtin_popcountll(t->piezas[c][caballo]) - __builtin_popcountll(t->piezas[!c][caballo])) + 
		(__builtin_popcountll(t->piezas[c][alfil]) - __builtin_popcountll(t->piezas[!c][alfil]))) +
		5 * (__builtin_popcountll(t->piezas[c][torre]) - __builtin_popcountll(t->piezas[!c][torre])) +
		9 * (__builtin_popcountll(t->piezas[c][reina]) - __builtin_popcountll(t->piezas[!c][reina])) +
		0.1 * (movePerColor[c].count - movePerColor[!c].count);

	float scaledValue = tanh(value / 25.0f);
	return scaledValue;
}
