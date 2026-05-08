#include "include/eval.h"
#include "include/bitboard.h"
#include "include/movegen.h"

float boardEval(Tablero * t, color c, int myMoveCount, int oppMoveCount) {
	if (t->piezas[c][rey] == 0)
		return -100000;

	int phase = 0;
	phase += 4 * (__builtin_popcountll(t->piezas[blancas][reina]) + __builtin_popcountll(t->piezas[negras][reina]));
	phase += 2 * (__builtin_popcountll(t->piezas[blancas][torre]) + __builtin_popcountll(t->piezas[negras][torre]));
	phase += (__builtin_popcountll(t->piezas[blancas][alfil] | t->piezas[blancas][caballo]) +
		  __builtin_popcountll(t->piezas[negras][alfil] | t->piezas[negras][caballo]));
	if (phase > 24)
		phase = 24;

	float mg = 0, eg = 0;

	const int val[6] = {100, 320, 330, 500, 900, 0};

	for (int p = peon; p <= reina; p++) {
		int own = __builtin_popcountll(t->piezas[c][p]);
		int opp = __builtin_popcountll(t->piezas[!c][p]);
		mg += val[p] * (own - opp);
		eg += val[p] * (own - opp);
	}

	for (int p = peon; p <= rey; p++) {
		bitboard bb = t->piezas[c][p];
		while (bb) {
			int sq = __builtin_ctzll(bb);
			bb &= bb - 1;
			mg += positionalValues[c][p][sq];
			eg += positionalValues[c][p][sq];
		}
		bb = t->piezas[!c][p];
		while (bb) {
			int sq = __builtin_ctzll(bb);
			bb &= bb - 1;
			mg -= positionalValues[!c][p][sq];
			eg -= positionalValues[!c][p][sq];
		}
	}

	if (myMoveCount == -1 || oppMoveCount == -1) {
		moveLists m;
		if (myMoveCount == -1) {
			generateAllPseudoMoves(c, t, &m);
			myMoveCount = m.count;
		}
		if (oppMoveCount == -1) {
			generateAllPseudoMoves(!c, t, &m);
			oppMoveCount = m.count;
		}
	}

	mg += 5 * (myMoveCount - oppMoveCount);
	eg += 2 * (myMoveCount - oppMoveCount);

	int kingSq = __builtin_ctzll(t->piezas[c][rey]);
	bitboard zone = kingAttacks[kingSq] | BB_SQUARE(kingSq);

	int attackers = __builtin_popcountll(zone & t->allPieces[!c]);

	int enemyMaterial =
	    900 * __builtin_popcountll(t->piezas[!c][reina]) + 500 * __builtin_popcountll(t->piezas[!c][torre]);

	mg -= attackers * (enemyMaterial > 0 ? 40 : 10);

	int shield = 0;
	if (c == blancas) {
		bitboard pawns = t->piezas[c][peon] & (BB_SQUARE(f2) | BB_SQUARE(g2) | BB_SQUARE(h2));
		shield = 3 - __builtin_popcountll(pawns);
	} else {
		bitboard pawns = t->piezas[c][peon] & (BB_SQUARE(f7) | BB_SQUARE(g7) | BB_SQUARE(h7));
		shield = 3 - __builtin_popcountll(pawns);
	}
	mg -= shield * 20;

	int pawnScore = 0;

	for (int file = 0; file < 8; file++) {
		bitboard own = t->piezas[c][peon] & BB_FILE(file);
		bitboard opp = t->piezas[!c][peon] & BB_FILE(file);

		int ownCount = __builtin_popcountll(own);
		int oppCount = __builtin_popcountll(opp);

		if (ownCount > 1)
			pawnScore -= 20 * (ownCount - 1);
		if (oppCount > 1)
			pawnScore += 20 * (oppCount - 1);

		bitboard left = (file > 0) ? t->piezas[c][peon] & BB_FILE(file - 1) : 0;
		bitboard right = (file < 7) ? t->piezas[c][peon] & BB_FILE(file + 1) : 0;
		if (own && !left && !right)
			pawnScore -= 15 * ownCount;

		left = (file > 0) ? t->piezas[!c][peon] & BB_FILE(file - 1) : 0;
		right = (file < 7) ? t->piezas[!c][peon] & BB_FILE(file + 1) : 0;
		if (opp && !left && !right)
			pawnScore += 15 * oppCount;
	}

	bitboard pawns = t->piezas[c][peon];
	while (pawns) {
		int sq = __builtin_ctzll(pawns);
		pawns &= pawns - 1;

		int rank = sq / 8;
		int file = sq % 8;

		int passed = 1;
		for (int f = (file > 0 ? file - 1 : file); f <= (file < 7 ? file + 1 : file); f++) {
			if (t->piezas[!c][peon] & BB_FILE(f)) {
				passed = 0;
				break;
			}
		}

		if (passed) {
			int r = (c == blancas) ? rank : (7 - rank);
			pawnScore += 20 + r * r * 5;
			eg += 10 + r * r * 8;
		}
	}

	mg += pawnScore;
	eg += pawnScore;

	for (int file = 0; file < 8; file++) {
		bitboard mask = BB_FILE(file);

		if (!(t->piezas[c][peon] & mask))
			mg += 20 * __builtin_popcountll(t->piezas[c][torre] & mask);

		if (!(t->piezas[!c][peon] & mask))
			mg -= 20 * __builtin_popcountll(t->piezas[!c][torre] & mask);
	}

	if (__builtin_popcountll(t->piezas[c][alfil]) >= 2)
		mg += 50;
	if (__builtin_popcountll(t->piezas[!c][alfil]) >= 2)
		mg -= 50;

	bitboard center = BB_SQUARE(d4) | BB_SQUARE(e4) | BB_SQUARE(d5) | BB_SQUARE(e5);

	mg += 10 * __builtin_popcountll(t->piezas[c][peon] & center);
	mg -= 10 * __builtin_popcountll(t->piezas[!c][peon] & center);

	bitboard attacked = t->allPieces[c] & t->allPieces[!c];
	mg -= 10 * __builtin_popcountll(attacked);

	float value = (mg * phase + eg * (24 - phase)) / 24;

	return value;
}