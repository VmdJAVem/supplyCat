#include "include/board.h"
#include "include/bitboard.h"

const uint8_t WHITE_OO = 1;
const uint8_t WHITE_OOO = 2;
const uint8_t BLACK_OO = 4;
const uint8_t BLACK_OOO = 8;

int sortingValues[6] = {100, 300, 350, 500, 900};

Tablero tablero = {0};
volatile bool stopRequested = false;
bool isPlaying = true;
color colorToMove;
bool debug = false;
Move killerMoves[MAX_DEPTH][2] = {0};
int history[64][64] = {0};
uint64_t positionHashes[MAX_GAME_LENGTH] = {0};
int positionHashCount = 0;
Zobrist zobrist;
goParameters parameters = {
    .wtime = -1,
    .btime = -1,
    .winc = -1,
    .binc = -1,
    .movestogo = -1,
    .depth = -1,
    .movetime = -1,
    .infinite = false,
};
long long nodes = 0;
int searchAge = 0;

int positionalValues[2][6][64] = {
    {
     {0,  0,  0,  0,  0,  0,  0,  0,  50, 50, 50, 50, 50, 50, 50, 50, 30, 30, 35, 40, 40, 35,
      30, 30, 20, 20, 25, 30, 30, 25, 20, 20, 10, 10, 15, 20, 20, 15, 10, 10, 5,  5,  10, 10,
      10, 10, 5,  5,  0,  0,  0,  5,  5,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
     {-60, -50, -40, -30, -30, -40, -50, -60, -50, -30, -10, 0,   0,   -10, -30, -50, -40, -10, 10,  20,  20, 10,
      -10, -40, -30, 0,   20,  30,  30,  20,  0,   -30, -30, 0,   20,  30,  30,  20,  0,   -30, -40, -10, 10, 20,
      20,  10, -10, -40, -50, -30, -10, 0,   0,   -10, -30, -50, -60, -50, -40, -30, -30, -40, -50, -60},
     {-20, -10, -10, -10, -10, -10, -10, -20, -10, 0,  0,   0,   0,   0,   0,  -10, -10, 0,  5,   10, 10, 5,
      0,   -10, -10, 5,   5,   10,  10,  5,   5,   -10, -10, 0,   10,  10,  10,  10,  0,   -10, -10, 10, 10, 10,
      10,  10, 10,  -10, -10, 5,   0,   0,   0,   0,   5,   -10, -20, -10, -10, -10, -10, -10, -10, -20},
     {0,  0, 0, 0, 0, 0, 0, 0,  5,  10, 10, 10, 10, 10, 10, 5,  -5, 0, 0, 0, 0, 0, 0, -5, -5, 0, 0, 0, 0, 0, 0, -5,
      -5, 0, 0, 0, 0, 0, 0, -5, -5, 0,  0,  0,  0,  0,  0,  -5, -5, 0, 0, 0, 0, 0, 0, -5, 0,  0, 0, 5, 5, 0, 0, 0},
     {-20, -10, -10, -5,  -5,  -10, -10, -20, -10, 0,  0, 0,   0,   0,   0,   -10, -10, 0,   5,   5,  5, 5,
      0,   -10, -5,  0,   5,   5,   5,   5,   0,   -5, 0, 0,   5,   5,   5,   5,   0,  -5,  -10, 5,  5, 5,
      5,   5,  0,   -10, -10, 0,   5,   0,   0,   0,  0, -10, -20, -10, -10, -5,  -5,  -10, -10, -20},
     {-30, -40, -40, -50, -50, -40, -40, -30, -30, -40, -40, -50, -50, -40, -40, -30, -30, -40, -40, -50, -50, -40,
      -40, -30, -30, -40, -40, -50, -50, -40, -40, -30, -20, -30, -30, -40, -40, -30, -30, -20, -10, -20, -20, -20,
      -20, -20, -20, -10, 20,  20,  0,   0,   0,   0,  20,  20,  20,  30,  10,  0,   0,   10,  30,  20}},
    {
     {0,  0,  0,  0,  0,  0,  0,  0,  50, 50, 50, 50, 50, 50, 50, 50, 30, 30, 35, 40, 40, 35,
      30, 30, 20, 20, 25, 30, 30, 25, 20, 20, 10, 10, 15, 20, 20, 15, 10, 10, 5,  5,  10, 10,
      10, 10, 5,  5,  0,  0,  0,  5,  5,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
     {-60, -50, -40, -30, -30, -40, -50, -60, -50, -30, -10, 0,   0,   -10, -30, -50, -40, -10, 10,  20,  20, 10,
      -10, -40, -30, 0,   20,  30,  30,  20,  0,   -30, -30, 0,   20,  30,  30,  20,  0,   -30, -40, -10, 10, 20,
      20,  10, -10, -40, -50, -30, -10, 0,   0,   -10, -30, -50, -60, -50, -40, -30, -30, -40, -50, -60},
     {-20, -10, -10, -10, -10, -10, -10, -20, -10, 0,  0,   0,   0,   0,   0,  -10, -10, 0,  5,   10, 10, 5,
      0,   -10, -10, 5,   5,   10,  10,  5,   5,   -10, -10, 0,   10,  10,  10,  10,  0,   -10, -10, 10, 10, 10,
      10,  10, 10,  -10, -10, 5,   0,   0,   0,   0,   5,   -10, -20, -10, -10, -10, -10, -10, -10, -20},
     {0,  0, 0, 5, 5, 0, 0, 0,  -5, 0, 0, 0, 0, 0, 0, -5, -5, 0,  0,  0,  0,  0,  0,  -5, -5, 0, 0, 0, 0, 0, 0, -5,
      -5, 0, 0, 0, 0, 0, 0, -5, -5, 0, 0, 0, 0, 0, 0, -5, 5,  10, 10, 10, 10, 10, 10, 5,  0,  0, 0, 0, 0, 0, 0, 0},
     {-20, -10, -10, -5,  -5,  -10, -10, -20, -10, 0,  0, 0,   0,   0,   0,   -10, -10, 0,   5,   5,  5, 5,
      0,   -10, -5,  0,   5,   5,   5,   5,   0,   -5, 0, 0,   5,   5,   5,   5,   0,  -5,  -10, 5,  5, 5,
      5,   5,  0,   -10, -10, 0,   5,   0,   0,   0,  0, -10, -20, -10, -10, -5,  -5,  -10, -10, -20},
     {-30, -40, -40, -50, -50, -40, -40, -30, -30, -40, -40, -50, -50, -40, -40, -30, -30, -40, -40, -50, -50, -40,
      -40, -30, -30, -40, -40, -50, -50, -40, -40, -30, -20, -30, -30, -40, -40, -30, -30, -20, -10, -20, -20, -20,
      -20, -20, -20, -10, 20,  20,  0,   0,   0,   0,  20,  20,  20,  30,  10,  0,   0,   10,  30,  20}}};

bool isRepetition(uint64_t hash) {
	for (int i = positionHashCount - 2; i >= 0; i -= 2) {
		if (positionHashes[i] == hash)
			return true;
		if (i == 0)
			break;
	}
	return false;
}

void initBoard(Tablero * t) {
	if (debug) {
		printf("DEBUG: initBoard called\n");
	}
	memset(t, 0, sizeof(Tablero));

	t->piezas[blancas][peon] = RANK(2 - 1);
	t->piezas[blancas][caballo] = BB_SQUARE(b1) | BB_SQUARE(g1);
	t->piezas[blancas][alfil] = BB_SQUARE(c1) | BB_SQUARE(f1);
	t->piezas[blancas][torre] = BB_SQUARE(a1) | BB_SQUARE(h1);
	t->piezas[blancas][reina] = BB_SQUARE(d1);
	t->piezas[blancas][rey] = BB_SQUARE(e1);

	t->piezas[negras][peon] = RANK(7 - 1);
	t->piezas[negras][caballo] = BB_SQUARE(b8) | BB_SQUARE(g8);
	t->piezas[negras][alfil] = BB_SQUARE(c8) | BB_SQUARE(f8);
	t->piezas[negras][torre] = BB_SQUARE(a8) | BB_SQUARE(h8);
	t->piezas[negras][reina] = BB_SQUARE(d8);
	t->piezas[negras][rey] = BB_SQUARE(e8);
	t->castlingRights = WHITE_OO | WHITE_OOO | BLACK_OO | BLACK_OOO;
	t->hash = computeZobrist(&zobrist, t, blancas);
	updateBoardCache(t);
	if (debug) {
		printBitboard(t->allOccupiedSquares);
	}
}

void updateBoardCache(Tablero * t) {
	t->allPieces[blancas] = t->piezas[blancas][peon] | t->piezas[blancas][caballo] | t->piezas[blancas][alfil] |
				t->piezas[blancas][torre] | t->piezas[blancas][reina] | t->piezas[blancas][rey];

	t->allPieces[negras] = t->piezas[negras][peon] | t->piezas[negras][caballo] | t->piezas[negras][alfil] |
			       t->piezas[negras][torre] | t->piezas[negras][reina] | t->piezas[negras][rey];
	t->allOccupiedSquares = t->allPieces[blancas] | t->allPieces[negras];
}

void makeMove(Move * move, Tablero * t, color c) {
	casilla enPassantSq = t->enPassantSquare;
	uint8_t oldCastling = t->castlingRights;

	if (t->enPassantSquare != -1) {
		int file = t->enPassantSquare % 8;
		t->hash ^= zobrist.enPassant[file];
	}
	t->enPassantSquare = -1;

	if (c == negras) {
		t->fullMoves++;
	}
	if (move->piece == peon) {
		t->halfmoveClock = 0;
	} else {
		t->halfmoveClock++;
	}

	switch (move->special) {
		case 0: {
			t->piezas[c][move->piece] &= ~(C64(1) << move->from);
			t->allPieces[c] &= ~(BB_SQUARE(move->from));
			t->allOccupiedSquares &= ~(BB_SQUARE(move->from));
			t->hash ^= zobrist.pieces[c][move->piece][move->from];

			if (move->capture != -1) {
				t->piezas[!c][move->capture] &= ~(C64(1) << move->to);
				t->allPieces[!c] &= ~(BB_SQUARE(move->to));
				t->allOccupiedSquares &= ~(BB_SQUARE(move->to));
				t->hash ^= zobrist.pieces[!c][move->capture][move->to];
			}

			t->piezas[c][move->piece] |= (C64(1) << move->to);
			t->allPieces[c] |= BB_SQUARE(move->to);
			t->allOccupiedSquares |= BB_SQUARE(move->to);
			t->hash ^= zobrist.pieces[c][move->piece][move->to];

			if (move->piece == rey) {
				if (c == blancas) {
					if (oldCastling & WHITE_OO)
						t->hash ^= zobrist.castling[0];
					if (oldCastling & WHITE_OOO)
						t->hash ^= zobrist.castling[1];
					t->castlingRights &= ~(WHITE_OO | WHITE_OOO);
				} else {
					if (oldCastling & BLACK_OO)
						t->hash ^= zobrist.castling[2];
					if (oldCastling & BLACK_OOO)
						t->hash ^= zobrist.castling[3];
					t->castlingRights &= ~(BLACK_OO | BLACK_OOO);
				}
			}
			else if (move->piece == torre) {
				if (move->from == h1 && (oldCastling & WHITE_OO)) {
					t->hash ^= zobrist.castling[0];
					t->castlingRights &= ~WHITE_OO;
				}
				if (move->from == a1 && (oldCastling & WHITE_OOO)) {
					t->hash ^= zobrist.castling[1];
					t->castlingRights &= ~WHITE_OOO;
				}
				if (move->from == h8 && (oldCastling & BLACK_OO)) {
					t->hash ^= zobrist.castling[2];
					t->castlingRights &= ~BLACK_OO;
				}
				if (move->from == a8 && (oldCastling & BLACK_OOO)) {
					t->hash ^= zobrist.castling[3];
					t->castlingRights &= ~BLACK_OOO;
				}
			}

			if (move->capture == torre) {
				if (move->to == h1 && (oldCastling & WHITE_OO)) {
					t->hash ^= zobrist.castling[0];
					t->castlingRights &= ~WHITE_OO;
				}
				if (move->to == a1 && (oldCastling & WHITE_OOO)) {
					t->hash ^= zobrist.castling[1];
					t->castlingRights &= ~WHITE_OOO;
				}
				if (move->to == h8 && (oldCastling & BLACK_OO)) {
					t->hash ^= zobrist.castling[2];
					t->castlingRights &= ~BLACK_OO;
				}
				if (move->to == a8 && (oldCastling & BLACK_OOO)) {
					t->hash ^= zobrist.castling[3];
					t->castlingRights &= ~BLACK_OOO;
				}
			}

			if (move->piece == peon &&
			    (move->to > move->from ? move->to - move->from : move->from - move->to) == 16) {
				if (c == blancas)
					t->enPassantSquare = move->to - 8;
				else
					t->enPassantSquare = move->to + 8;
				int file = t->enPassantSquare % 8;
				t->hash ^= zobrist.enPassant[file];
			}
			break;
		}

		case 1: {
			if (enPassantSq == -1) {
				fprintf(stderr, "Error: en passant move with no target square\n");
				return;
			}
			casilla opponentPawn = (c == blancas ? enPassantSq - 8 : enPassantSq + 8);

			t->piezas[c][move->piece] &= ~(C64(1) << move->from);
			t->allPieces[c] &= ~(BB_SQUARE(move->from));
			t->allOccupiedSquares &= ~(BB_SQUARE(move->from));
			t->hash ^= zobrist.pieces[c][move->piece][move->from];

			t->piezas[!c][peon] &= ~(C64(1) << opponentPawn);
			t->allPieces[!c] &= ~(BB_SQUARE(opponentPawn));
			t->allOccupiedSquares &= ~(BB_SQUARE(opponentPawn));
			t->hash ^= zobrist.pieces[!c][peon][opponentPawn];

			t->piezas[c][move->piece] |= (C64(1) << move->to);
			t->allPieces[c] |= BB_SQUARE(move->to);
			t->allOccupiedSquares |= BB_SQUARE(move->to);
			t->hash ^= zobrist.pieces[c][move->piece][move->to];

			break;
		}

		case 2: {
			t->piezas[c][rey] &= ~(C64(1) << move->from);
			t->piezas[c][rey] |= (C64(1) << move->to);
			t->allPieces[c] &= ~(BB_SQUARE(move->from));
			t->allPieces[c] |= BB_SQUARE(move->to);
			t->allOccupiedSquares &= ~(BB_SQUARE(move->from));
			t->allOccupiedSquares |= BB_SQUARE(move->to);
			t->hash ^= zobrist.pieces[c][rey][move->from];
			t->hash ^= zobrist.pieces[c][rey][move->to];

			casilla rookFrom, rookTo;
			if (move->to == g1) {
				rookFrom = h1;
				rookTo = f1;
			} else if (move->to == c1) {
				rookFrom = a1;
				rookTo = d1;
			} else if (move->to == g8) {
				rookFrom = h8;
				rookTo = f8;
			} else if (move->to == c8) {
				rookFrom = a8;
				rookTo = d8;
			} else
				break;

			t->piezas[c][torre] &= ~(BB_SQUARE(rookFrom));
			t->piezas[c][torre] |= BB_SQUARE(rookTo);
			t->allPieces[c] &= ~(BB_SQUARE(rookFrom));
			t->allPieces[c] |= BB_SQUARE(rookTo);
			t->allOccupiedSquares &= ~(BB_SQUARE(rookFrom));
			t->allOccupiedSquares |= BB_SQUARE(rookTo);
			t->hash ^= zobrist.pieces[c][torre][rookFrom];
			t->hash ^= zobrist.pieces[c][torre][rookTo];

			if (c == blancas) {
				if (oldCastling & WHITE_OO)
					t->hash ^= zobrist.castling[0];
				if (oldCastling & WHITE_OOO)
					t->hash ^= zobrist.castling[1];
				t->castlingRights &= ~(WHITE_OO | WHITE_OOO);
			} else {
				if (oldCastling & BLACK_OO)
					t->hash ^= zobrist.castling[2];
				if (oldCastling & BLACK_OOO)
					t->hash ^= zobrist.castling[3];
				t->castlingRights &= ~(BLACK_OO | BLACK_OOO);
			}
			break;
		}

		case 3: {
			t->piezas[c][peon] &= ~(C64(1) << move->from);
			t->allPieces[c] &= ~(BB_SQUARE(move->from));
			t->allOccupiedSquares &= ~(BB_SQUARE(move->from));
			t->hash ^= zobrist.pieces[c][peon][move->from];

			if (move->capture != -1) {
				t->piezas[!c][move->capture] &= ~(C64(1) << move->to);
				t->allPieces[!c] &= ~(BB_SQUARE(move->to));
				t->allOccupiedSquares &= ~(BB_SQUARE(move->to));
				t->hash ^= zobrist.pieces[!c][move->capture][move->to];
			}

			t->piezas[c][move->promoPiece] |= (C64(1) << move->to);
			t->allPieces[c] |= BB_SQUARE(move->to);
			t->allOccupiedSquares |= BB_SQUARE(move->to);
			t->hash ^= zobrist.pieces[c][move->promoPiece][move->to];

			break;
		}
	}

	t->hash ^= zobrist.side;
}

void unmakeMove(Move * move, Tablero * t, color c, int oldEnPassant, uint8_t oldCastling, int oldHalfClock, int oldFullClock) {

	t->halfmoveClock = oldHalfClock;
	t->fullMoves = oldFullClock;

	switch (move->special) {

		case 0: {
			t->piezas[c][move->piece] &= ~BB_SQUARE(move->to);
			t->allPieces[c] &= ~BB_SQUARE(move->to);
			t->allOccupiedSquares &= ~BB_SQUARE(move->to);
			t->hash ^= zobrist.pieces[c][move->piece][move->to];

			if (move->capture != -1) {
				t->piezas[!c][move->capture] |= BB_SQUARE(move->to);
				t->allPieces[!c] |= BB_SQUARE(move->to);
				t->allOccupiedSquares |= BB_SQUARE(move->to);
				t->hash ^= zobrist.pieces[!c][move->capture][move->to];
			}

			t->piezas[c][move->piece] |= BB_SQUARE(move->from);
			t->allPieces[c] |= BB_SQUARE(move->from);
			t->allOccupiedSquares |= BB_SQUARE(move->from);
			t->hash ^= zobrist.pieces[c][move->piece][move->from];

			break;
		}

		case 1: {
			t->piezas[c][peon] &= ~BB_SQUARE(move->to);
			t->allPieces[c] &= ~BB_SQUARE(move->to);
			t->allOccupiedSquares &= ~BB_SQUARE(move->to);
			t->hash ^= zobrist.pieces[c][peon][move->to];

			t->piezas[c][peon] |= BB_SQUARE(move->from);
			t->allPieces[c] |= BB_SQUARE(move->from);
			t->allOccupiedSquares |= BB_SQUARE(move->from);
			t->hash ^= zobrist.pieces[c][peon][move->from];

			int capSq = (c == blancas) ? (move->to - 8) : (move->to + 8);
			t->piezas[!c][peon] |= BB_SQUARE(capSq);
			t->allPieces[!c] |= BB_SQUARE(capSq);
			t->allOccupiedSquares |= BB_SQUARE(capSq);
			t->hash ^= zobrist.pieces[!c][peon][capSq];

			break;
		}

		case 2: {
			t->piezas[c][rey] &= ~BB_SQUARE(move->to);
			t->allPieces[c] &= ~BB_SQUARE(move->to);
			t->allOccupiedSquares &= ~BB_SQUARE(move->to);
			t->hash ^= zobrist.pieces[c][rey][move->to];

			t->piezas[c][rey] |= BB_SQUARE(move->from);
			t->allPieces[c] |= BB_SQUARE(move->from);
			t->allOccupiedSquares |= BB_SQUARE(move->from);
			t->hash ^= zobrist.pieces[c][rey][move->from];

			casilla rookFrom, rookTo;

			if (move->to == g1) {
				rookFrom = f1;
				rookTo = h1;
			} else if (move->to == c1) {
				rookFrom = d1;
				rookTo = a1;
			} else if (move->to == g8) {
				rookFrom = f8;
				rookTo = h8;
			} else if (move->to == c8) {
				rookFrom = d8;
				rookTo = a8;
			} else
				break;

			t->piezas[c][torre] &= ~BB_SQUARE(rookFrom);
			t->allPieces[c] &= ~BB_SQUARE(rookFrom);
			t->allOccupiedSquares &= ~BB_SQUARE(rookFrom);
			t->hash ^= zobrist.pieces[c][torre][rookFrom];

			t->piezas[c][torre] |= BB_SQUARE(rookTo);
			t->allPieces[c] |= BB_SQUARE(rookTo);
			t->allOccupiedSquares |= BB_SQUARE(rookTo);
			t->hash ^= zobrist.pieces[c][torre][rookTo];

			break;
		}

		case 3: {
			t->piezas[c][move->promoPiece] &= ~BB_SQUARE(move->to);
			t->allPieces[c] &= ~BB_SQUARE(move->to);
			t->allOccupiedSquares &= ~BB_SQUARE(move->to);
			t->hash ^= zobrist.pieces[c][move->promoPiece][move->to];

			if (move->capture != -1) {
				t->piezas[!c][move->capture] |= BB_SQUARE(move->to);
				t->allPieces[!c] |= BB_SQUARE(move->to);
				t->allOccupiedSquares |= BB_SQUARE(move->to);
				t->hash ^= zobrist.pieces[!c][move->capture][move->to];
			}

			t->piezas[c][peon] |= BB_SQUARE(move->from);
			t->allPieces[c] |= BB_SQUARE(move->from);
			t->allOccupiedSquares |= BB_SQUARE(move->from);
			t->hash ^= zobrist.pieces[c][peon][move->from];

			break;
		}
	}

	if (t->castlingRights & WHITE_OO)
		t->hash ^= zobrist.castling[0];
	if (t->castlingRights & WHITE_OOO)
		t->hash ^= zobrist.castling[1];
	if (t->castlingRights & BLACK_OO)
		t->hash ^= zobrist.castling[2];
	if (t->castlingRights & BLACK_OOO)
		t->hash ^= zobrist.castling[3];

	t->castlingRights = oldCastling;

	if (oldCastling & WHITE_OO)
		t->hash ^= zobrist.castling[0];
	if (oldCastling & WHITE_OOO)
		t->hash ^= zobrist.castling[1];
	if (oldCastling & BLACK_OO)
		t->hash ^= zobrist.castling[2];
	if (oldCastling & BLACK_OOO)
		t->hash ^= zobrist.castling[3];

	if (t->enPassantSquare != -1) {
		t->hash ^= zobrist.enPassant[t->enPassantSquare % 8];
	}

	t->enPassantSquare = oldEnPassant;

	if (oldEnPassant != -1) {
		t->hash ^= zobrist.enPassant[oldEnPassant % 8];
	}

	t->hash ^= zobrist.side;
}

void initZobrist() {
	for (int i = 0; i < 2; i++) {
		for (int j = 0; j < 6; j++) {
			for (int k = 0; k < 64; k++) {
				zobrist.pieces[i][j][k] = rand();
			}
		}
	}
	for (int i = 0; i < 4; i++) {
		zobrist.castling[i] = rand();
	}
	for (int i = 0; i < 8; i++) {
		zobrist.enPassant[i] = rand();
	}
}

uint64_t computeZobrist(Zobrist * z, Tablero * t, color sideToMove) {
	uint64_t hash = 0;
	for (int c = 0; c < 2; c++) {
		for (int p = 0; p <= rey; p++) {
			bitboard q = t->piezas[c][p];
			while (q) {
				int s = __builtin_ctzll(q);
				q &= q - 1;
				hash ^= z->pieces[c][p][s];
			}
		}
	}
	hash ^= (sideToMove == negras ? z->side : 0);
	if (t->castlingRights & WHITE_OO) {
		hash ^= z->castling[0];
	}
	if (t->castlingRights & WHITE_OOO) {
		hash ^= z->castling[1];
	}
	if (t->castlingRights & BLACK_OO) {
		hash ^= z->castling[2];
	}
	if (t->castlingRights & BLACK_OOO) {
		hash ^= z->castling[3];
	}
	if (t->enPassantSquare != -1) {
		int file = t->enPassantSquare % 8;
		hash ^= z->enPassant[file];
	}
	return hash;
}

tipoDePieza charToPiece(char c) {
	switch (c) {
		case 'p':
		case 'P':
			return peon;
		case 'r':
		case 'R':
			return torre;
		case 'n':
		case 'N':
			return caballo;
		case 'b':
		case 'B':
			return alfil;
		case 'q':
		case 'Q':
			return reina;
		case 'k':
		case 'K':
			return rey;
	}
	return 0;
}

casilla stringToSq(const char * sq) {
	int file = tolower(sq[0]) - 'a';
	int rank = sq[1] - '1';
	return rank * 8 + file;
}

char pieceToChar(tipoDePieza piece) {
	switch (piece) {
		case peon:
			return 'p';
		case torre:
			return 'r';
		case caballo:
			return 'n';
		case alfil:
			return 'b';
		case reina:
			return 'q';
		case rey:
			return 'k';
	}
	printf("error: invalid fen\n");
	return 0;
}

char * moveToStr(Move * move) {
	static char result[6];
	int fromFile = move->from % 8;
	int fromRank = move->from / 8;

	int toFile = move->to % 8;
	int toRank = move->to / 8;

	result[0] = 'a' + fromFile;
	result[1] = '1' + fromRank;
	result[2] = 'a' + toFile;
	result[3] = '1' + toRank;
	if (move->special == 3) {
		result[4] = pieceToChar(move->promoPiece);
		result[5] = '\0';
	} else {
		result[4] = '\0';
	}
	return result;
}