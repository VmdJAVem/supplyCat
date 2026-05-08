#include "include/movegen.h"
#include "include/bitboard.h"
#include "include/board.h"

void generateAllMoves(color c, Tablero * t, moveLists * output) {
	output->count = 0;
	moveLists temp = {0};
	generateKingMoves(&temp, c, t);
	generateKnightMoves(&temp, c, t);
	generatePawnMoves(&temp, c, t);
	generateRookMoves(&temp, c, t);
	generateBishopMoves(&temp, c, t);
	generateQueenMoves(&temp, c, t);
	if (debug) {
		printf("temp.count = %d\n", temp.count);
	}
	for (int i = 0; i < temp.count; i++) {
		casilla oldEp = t->enPassantSquare;
		uint8_t oldCastling = t->castlingRights;
		int oldHalf = t->halfmoveClock;
		int oldFull = t->fullMoves;
		makeMove(&temp.moves[i], t, c);
		casilla king = __builtin_ctzll(t->piezas[c][rey]);
		if (!isAttacked(t, king, !c)) {
			output->moves[output->count] = temp.moves[i];
			output->count++;
			if (debug) {
				printf("move %s passed\n", moveToStr(&temp.moves[i]));
			}
		}
		unmakeMove(&temp.moves[i], t, c, oldEp, oldCastling, oldHalf, oldFull);
	}
}

void generateAllPseudoMoves(color c, Tablero * t, moveLists * output) {
	output->count = 0;
	generateKingMoves(output, c, t);
	generateKnightMoves(output, c, t);
	generatePawnMoves(output, c, t);
	generateRookMoves(output, c, t);
	generateBishopMoves(output, c, t);
	generateQueenMoves(output, c, t);
}

void generateLegalMoves(color c, Tablero * t, moveLists * output) {
	generateAllPseudoMoves(c, t, output);
	casilla kingSq = __builtin_ctzll(t->piezas[c][rey]);
	bool inCheck = isAttacked(t, kingSq, !c);
	if (!inCheck) {
		return;
	}
	int outCount = 0;
	for (int i = 0; i < output->count; i++) {
		casilla oldEp = t->enPassantSquare;
		uint8_t oldCastling = t->castlingRights;
		int oldHalf = t->halfmoveClock;
		int oldFull = t->fullMoves;
		makeMove(&output->moves[i], t, c);
		casilla king = __builtin_ctzll(t->piezas[c][rey]);
		if (!isAttacked(t, king, !c)) {
			output->moves[outCount++] = output->moves[i];
		}
		unmakeMove(&output->moves[i], t, c, oldEp, oldCastling, oldHalf, oldFull);
	}
	output->count = outCount;
}

void generateCaptures(color c, Tablero * t, moveLists * output) {
	output->count = 0;
	generatePawnCaptures(output, c, t);
	generateKnightCaptures(output, c, t);
	generateBishopCaptures(output, c, t);
	generateRookCaptures(output, c, t);
	generateQueenCaptures(output, c, t);
	generateKingCaptures(output, c, t);
}

void generateKnightMoves(moveLists * ml, color c, Tablero * t) {
	bitboard allKnights = t->piezas[c][caballo];
	while (allKnights) {
		casilla from = __builtin_ctzll(allKnights);
		allKnights &= (allKnights - 1);
		bitboard attacks = (knightAttacks[from] & (~t->allPieces[c]));
		while (attacks) {
			casilla to = __builtin_ctzll(attacks);
			attacks &= (attacks - 1);
			int capture = -1;
			if (BB_SQUARE(to) & t->allPieces[!c]) {
				for (int piece = peon; piece <= rey; piece++) {
					if (t->piezas[!c][piece] & BB_SQUARE(to)) {
						capture = piece;
						break;
					}
				}
			}
			Move move = {from, to, caballo, capture, 0, 0};
			ml->moves[ml->count] = move;
			ml->count++;
		}
	}
}

void generateKingMoves(moveLists * ml, color c, Tablero * t) {
	bitboard king = t->piezas[c][rey];
	if (king == 0)
		return;
	if (king & (king - 1))
		return;
	casilla from = __builtin_ctzll(king);
	bitboard attacks = kingAttacks[from] & ~t->allPieces[c];
	while (attacks) {
		casilla to = __builtin_ctzll(attacks);
		attacks &= attacks - 1;
		int capture = -1;
		if (BB_SQUARE(to) & t->allPieces[!c]) {
			for (int piece = peon; piece <= rey; piece++) {
				if (t->piezas[!c][piece] & BB_SQUARE(to)) {
					capture = piece;
					break;
				}
			}
		}
		Move move = {from, to, rey, capture, 0, 0};
		ml->moves[ml->count++] = move;
	}

	bitboard attacked = attackedByColor(t, !c);
	if (c == blancas && from == e1) {
		bitboard rooks = t->piezas[c][torre];
		while (rooks) {
			bool canCastleQueenSide = true;
			bool canCastleKingSide = true;
			casilla rook = __builtin_ctzll(rooks);
			rooks &= (rooks - 1);
			if (rook == h1 && (t->castlingRights & WHITE_OO)) {
				for (casilla sq = f1; sq < h1; sq++) {
					if ((BB_SQUARE(sq) & (t->allPieces[c] | t->allPieces[!c])) ||
					    BB_SQUARE(sq) & attacked) {
						canCastleKingSide = false;
					}
				}
				if (canCastleKingSide &&
				    !(BB_SQUARE(from) & attacked ||
				      BB_SQUARE(g1) & attacked)) {
					casilla to = g1;
					Move move = {from, to, rey, -1, 2, 0};
					ml->moves[ml->count] = move;
					ml->count++;
				}
			} else if (rook == a1 && (t->castlingRights & WHITE_OOO)) {
				for (casilla sq = d1; sq > a1; sq--) {
					if ((BB_SQUARE(sq) & (t->allPieces[c] | t->allPieces[!c])) ||
					    BB_SQUARE(sq) & attacked) {
						canCastleQueenSide = false;
					}
				}
				if (canCastleQueenSide && !(BB_SQUARE(c1) & attacked || BB_SQUARE(from) & attacked)) {
					casilla to = c1;
					Move move = {from, to, rey, -1, 2, 0};
					ml->moves[ml->count] = move;
					ml->count++;
				}
			}
		}
	}
	if (c == negras && from == e8) {
		bitboard rooks = t->piezas[c][torre];
		while (rooks) {
			casilla rook = __builtin_ctzll(rooks);
			rooks &= (rooks - 1);
			bool canCastleQueenSide = true;
			bool canCastleKingSide = true;
			if (rook == h8 && (t->castlingRights & BLACK_OO)) {
				for (casilla sq = f8; sq < h8; sq++) {
					if ((BB_SQUARE(sq) & (t->allPieces[c] | t->allPieces[!c])) ||
					    isAttacked(t, sq, !c)) {
						canCastleKingSide = false;
					}
				}

				if (canCastleKingSide && !(isAttacked(t, from, !c) || isAttacked(t, g8, !c))) {
					casilla to = g8;
					Move move = {from, to, rey, -1, 2, 0};
					ml->moves[ml->count] = move;
					ml->count++;
				}
			} else if (rook == a8 && (t->castlingRights & BLACK_OOO)) {
				for (casilla sq = d8; sq > a8; sq--) {
					if ((BB_SQUARE(sq) & (t->allPieces[c] | t->allPieces[!c])) ||
					    isAttacked(t, sq, !c)) {
						canCastleQueenSide = false;
					}
				}
				if (canCastleQueenSide && !(isAttacked(t, from, !c) || isAttacked(t, c8, !c))) {
					casilla to = c8;
					Move move = {from, to, rey, -1, 2, 0};
					ml->moves[ml->count] = move;
					ml->count++;
				}
			}
		}
	}
}

void generatePawnMoves(moveLists * ml, color c, Tablero * t) {
	bitboard allPawns = t->piezas[c][peon];
	while (allPawns) {
		casilla from = __builtin_ctzll(allPawns);
		allPawns &= (allPawns - 1);
		casilla to = 0;
		casilla rank = from / 8;
		bitboard pawnAttacksLocal = pawnAttacks[c][from];
		if (c == blancas) {
			to = from + 8;
		} else if (c == negras) {
			to = from - 8;
		}
		if (to < 64 && to >= 0) {
			if (BB_SQUARE(to) & (~t->allOccupiedSquares)) {
				if ((to / 8 == 0 && c == negras) || (to / 8 == 7 && c == blancas)) {
					for (int i = caballo; i < rey; i++) {
						Move move = {from, to, peon, -1, 3, i};
						ml->moves[ml->count] = move;
						ml->count++;
					}
				} else {
					Move move = {from, to, peon, -1, 0, 0};
					ml->moves[ml->count] = move;
					ml->count++;
				}
			}
		}
		if (c == blancas && rank == 1) {
			if (!(t->allOccupiedSquares & BB_SQUARE(from + 8)) &&
			    !(t->allOccupiedSquares & BB_SQUARE(from + 16))) {
				Move move = {from, from + 16, peon, -1, 0, 0};
				ml->moves[ml->count++] = move;
			}
		} else if (c == negras && rank == 6) {
			if (!(t->allOccupiedSquares & BB_SQUARE(from - 8)) &&
			    !(t->allOccupiedSquares & BB_SQUARE(from - 16))) {
				Move move = {from, from - 16, peon, -1, 0, 0};
				ml->moves[ml->count++] = move;
			}
		}
		if (pawnAttacksLocal & t->allPieces[!c]) {
			pawnAttacksLocal &= t->allPieces[!c];
			while (pawnAttacksLocal) {
				int capture = -1;
				if (pawnAttacksLocal & t->allPieces[!c]) {
					to = __builtin_ctzll(pawnAttacksLocal);
					pawnAttacksLocal &= (pawnAttacksLocal - 1);
					for (int piece = peon; piece <= rey; piece++) {
						if (t->piezas[!c][piece] & BB_SQUARE(to)) {
							capture = piece;
							break;
						}
					}
					Move move = {from, to, peon, capture, 0, 0};
					ml->moves[ml->count] = move;
					ml->count++;
				}
			}
		}
		if (rank == 4 && t->enPassantSquare != -1 && c == blancas) {
			int to1 = from + 7;
			int to2 = from + 9;
			if (to1 >= 0 && to1 < 64 && t->enPassantSquare == to1) {
				Move move = {from, to1, peon, 0, 1, 0};
				ml->moves[ml->count++] = move;
			}
			if (to2 >= 0 && to2 < 64 && t->enPassantSquare == to2) {
				Move move = {from, to2, peon, 0, 1, 0};
				ml->moves[ml->count++] = move;
			}
		}
		else if (rank == 3 && t->enPassantSquare != -1 && c == negras) {
			int to1 = from - 7;
			int to2 = from - 9;
			if (to1 >= 0 && to1 < 64 && t->enPassantSquare == to1) {
				Move move = {from, to1, peon, 0, 1, 0};
				ml->moves[ml->count++] = move;
			}
			if (to2 >= 0 && to2 < 64 && t->enPassantSquare == to2) {
				Move move = {from, to2, peon, 0, 1, 0};
				ml->moves[ml->count++] = move;
			}
		}
	}
}

void generateRookMoves(moveLists * ml, color c, Tablero * t) {
	bitboard rooks = t->piezas[c][torre];

	while (rooks) {
		int from = __builtin_ctzll(rooks);
		rooks &= rooks - 1;

		for (int d = 0; d < 4; d++) {
			bitboard ray = rookMask[from][d];
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

				bitboard quiet = before & ~t->allPieces[!c];
				while (quiet) {
					int to = __builtin_ctzll(quiet);
					quiet &= quiet - 1;
					Move move = {from, to, torre, -1, 0, 0};
					ml->moves[ml->count++] = move;
				}

				if (BB_SQUARE(firstBlocker) & t->allPieces[!c]) {
					int capture = -1;
					for (int p = peon; p <= rey; p++) {
						if (t->piezas[!c][p] & BB_SQUARE(firstBlocker)) {
							capture = p;
							break;
						}
					}
					Move move = {from, firstBlocker, torre, capture, 0, 0};
					ml->moves[ml->count++] = move;
				}
			} else {
				while (ray) {
					int to = __builtin_ctzll(ray);
					ray &= ray - 1;
					Move move = {from, to, torre, -1, 0, 0};
					ml->moves[ml->count++] = move;
				}
			}
		}
	}
}

void generateBishopMoves(moveLists * ml, color c, Tablero * t) {
	bitboard bishops = t->piezas[c][alfil];

	while (bishops) {
		int from = __builtin_ctzll(bishops);
		bishops &= bishops - 1;

		for (int d = 0; d < 4; d++) {
			bitboard ray = bishopMask[from][d];
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

				bitboard quiet = before & ~t->allPieces[!c];
				while (quiet) {
					int to = __builtin_ctzll(quiet);
					quiet &= quiet - 1;
					Move move = {from, to, alfil, -1, 0, 0};
					ml->moves[ml->count++] = move;
				}

				if (BB_SQUARE(firstBlocker) & t->allPieces[!c]) {
					int capture = -1;
					for (int p = peon; p <= rey; p++) {
						if (t->piezas[!c][p] & BB_SQUARE(firstBlocker)) {
							capture = p;
							break;
						}
					}
					Move move = {from, firstBlocker, alfil, capture, 0, 0};
					ml->moves[ml->count++] = move;
				}
			} else {
				while (ray) {
					int to = __builtin_ctzll(ray);
					ray &= ray - 1;
					Move move = {from, to, alfil, -1, 0, 0};
					ml->moves[ml->count++] = move;
				}
			}
		}
	}
}

void generateQueenMoves(moveLists * ml, color c, Tablero * t) {
	bitboard queens = t->piezas[c][reina];

	while (queens) {
		int from = __builtin_ctzll(queens);
		queens &= queens - 1;

		for (int d = 0; d < 4; d++) {
			bitboard ray = bishopMask[from][d];
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

				bitboard quiet = before & ~t->allPieces[!c];
				while (quiet) {
					int to = __builtin_ctzll(quiet);
					quiet &= quiet - 1;
					Move move = {from, to, reina, -1, 0, 0};
					ml->moves[ml->count++] = move;
				}

				if (BB_SQUARE(firstBlocker) & t->allPieces[!c]) {
					int capture = -1;
					for (int p = peon; p <= rey; p++) {
						if (t->piezas[!c][p] & BB_SQUARE(firstBlocker)) {
							capture = p;
							break;
						}
					}
					Move move = {from, firstBlocker, reina, capture, 0, 0};
					ml->moves[ml->count++] = move;
				}
			} else {
				while (ray) {
					int to = __builtin_ctzll(ray);
					ray &= ray - 1;
					Move move = {from, to, reina, -1, 0, 0};
					ml->moves[ml->count++] = move;
				}
			}
		}

		for (int d = 0; d < 4; d++) {
			bitboard ray = rookMask[from][d];
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

				bitboard quiet = before & ~t->allPieces[!c];
				while (quiet) {
					int to = __builtin_ctzll(quiet);
					quiet &= quiet - 1;
					Move move = {from, to, reina, -1, 0, 0};
					ml->moves[ml->count++] = move;
				}

				if (BB_SQUARE(firstBlocker) & t->allPieces[!c]) {
					int capture = -1;
					for (int p = peon; p <= rey; p++) {
						if (t->piezas[!c][p] & BB_SQUARE(firstBlocker)) {
							capture = p;
							break;
						}
					}
					Move move = {from, firstBlocker, reina, capture, 0, 0};
					ml->moves[ml->count++] = move;
				}
			} else {
				while (ray) {
					int to = __builtin_ctzll(ray);
					ray &= ray - 1;
					Move move = {from, to, reina, -1, 0, 0};
					ml->moves[ml->count++] = move;
				}
			}
		}
	}
}

void generateRookCaptures(moveLists * ml, color c, Tablero * t) {
	bitboard rooks = t->piezas[c][torre];

	while (rooks) {
		int from = __builtin_ctzll(rooks);
		rooks &= rooks - 1;

		for (int d = 0; d < 4; d++) {
			bitboard ray = rookMask[from][d];
			bitboard blockers = ray & t->allOccupiedSquares;

			if (blockers) {
				int firstBlocker;
				if (d < 2)
					firstBlocker = __builtin_ctzll(blockers);
				else
					firstBlocker = 63 - __builtin_clzll(blockers);

				if (BB_SQUARE(firstBlocker) & t->allPieces[!c]) {
					int capture = -1;
					for (int p = peon; p <= rey; p++) {
						if (t->piezas[!c][p] & BB_SQUARE(firstBlocker)) {
							capture = p;
							break;
						}
					}
					Move move = {from, firstBlocker, torre, capture, 0, 0};
					ml->moves[ml->count++] = move;
				}
			}
		}
	}
}

void generateBishopCaptures(moveLists * ml, color c, Tablero * t) {
	bitboard bishops = t->piezas[c][alfil];

	while (bishops) {
		int from = __builtin_ctzll(bishops);
		bishops &= bishops - 1;

		for (int d = 0; d < 4; d++) {
			bitboard ray = bishopMask[from][d];
			bitboard blockers = ray & t->allOccupiedSquares;

			if (blockers) {
				int firstBlocker;
				if (d > 1)
					firstBlocker = __builtin_ctzll(blockers);
				else
					firstBlocker = 63 - __builtin_clzll(blockers);

				if (BB_SQUARE(firstBlocker) & t->allPieces[!c]) {
					int capture = -1;
					for (int p = peon; p <= rey; p++) {
						if (t->piezas[!c][p] & BB_SQUARE(firstBlocker)) {
							capture = p;
							break;
						}
					}
					Move move = {from, firstBlocker, alfil, capture, 0, 0};
					ml->moves[ml->count++] = move;
				}
			}
		}
	}
}

void generateKnightCaptures(moveLists * ml, color c, Tablero * t) {
	bitboard allKnights = t->piezas[c][caballo];
	while (allKnights) {
		casilla from = __builtin_ctzll(allKnights);
		allKnights &= allKnights - 1;
		bitboard attacks = knightAttacks[from];
		while (attacks) {
			casilla to = __builtin_ctzll(attacks);
			attacks &= attacks - 1;
			if (BB_SQUARE(to) & t->allPieces[!c]) {
				int capture = -1;
				for (int p = peon; p <= rey; p++) {
					if (t->piezas[!c][p] & BB_SQUARE(to)) {
						capture = p;
						break;
					}
				}
				Move move = {from, to, caballo, capture, 0, 0};
				ml->moves[ml->count++] = move;
			}
		}
	}
}

void generatePawnCaptures(moveLists * ml, color c, Tablero * t) {
	bitboard allPawns = t->piezas[c][peon];
	while (allPawns) {
		casilla from = __builtin_ctzll(allPawns);
		allPawns &= allPawns - 1;
		bitboard attacks = pawnAttacks[c][from] & t->allPieces[!c];
		while (attacks) {
			casilla to = __builtin_ctzll(attacks);
			attacks &= attacks - 1;
			int capture = -1;
			for (int p = peon; p <= rey; p++) {
				if (t->piezas[!c][p] & BB_SQUARE(to)) {
					capture = p;
					break;
				}
			}
			Move move = {from, to, peon, capture, 0, 0};
			ml->moves[ml->count++] = move;
		}
	}
}

void generateQueenCaptures(moveLists * ml, color c, Tablero * t) {
	bitboard queens = t->piezas[c][reina];

	while (queens) {
		int from = __builtin_ctzll(queens);
		queens &= queens - 1;

		for (int d = 0; d < 4; d++) {
			bitboard ray = rookMask[from][d];
			bitboard blockers = ray & t->allOccupiedSquares;

			if (blockers) {
				int firstBlocker;
				if (d < 2)
					firstBlocker = __builtin_ctzll(blockers);
				else
					firstBlocker = 63 - __builtin_clzll(blockers);

				if (BB_SQUARE(firstBlocker) & t->allPieces[!c]) {
					int capture = -1;
					for (int p = peon; p <= rey; p++) {
						if (t->piezas[!c][p] & BB_SQUARE(firstBlocker)) {
							capture = p;
							break;
						}
					}
					Move move = {from, firstBlocker, reina, capture, 0, 0};
					ml->moves[ml->count++] = move;
				}
			}
		}

		for (int d = 0; d < 4; d++) {
			bitboard ray = bishopMask[from][d];
			bitboard blockers = ray & t->allOccupiedSquares;

			if (blockers) {
				int firstBlocker;
				if (d > 1)
					firstBlocker = __builtin_ctzll(blockers);
				else
					firstBlocker = 63 - __builtin_clzll(blockers);

				if (BB_SQUARE(firstBlocker) & t->allPieces[!c]) {
					int capture = -1;
					for (int p = peon; p <= rey; p++) {
						if (t->piezas[!c][p] & BB_SQUARE(firstBlocker)) {
							capture = p;
							break;
						}
					}
					Move move = {from, firstBlocker, reina, capture, 0, 0};
					ml->moves[ml->count++] = move;
				}
			}
		}
	}
}

void generateKingCaptures(moveLists * ml, color c, Tablero * t) {
	bitboard king = t->piezas[c][rey];
	if (!king)
		return;
	casilla from = __builtin_ctzll(king);
	bitboard attacks = kingAttacks[from] & ~t->allPieces[c];
	while (attacks) {
		casilla to = __builtin_ctzll(attacks);
		attacks &= attacks - 1;
		if (BB_SQUARE(to) & t->allPieces[!c]) {
			int capture = -1;
			for (int p = peon; p <= rey; p++) {
				if (t->piezas[!c][p] & BB_SQUARE(to)) {
					capture = p;
					break;
				}
			}
			Move move = {from, to, rey, capture, 0, 0};
			ml->moves[ml->count++] = move;
		}
	}
}