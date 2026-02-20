#define _POSIX_C_SOURCE 199309L
#include "move.h"
#include "board.h"
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>

void makeMove(Move *move, Tablero *t, color c) {
	casilla enPassantSq = t->enPassantSquare;
	t->enPassantSquare = -1;
	t->fullMoves++;

	if (move->piece == peon) {
		t->halfmoveClock = 0;
	}

	switch (move->special) {
		case 0:
			if (move->capture != 0) {
				t->piezas[!c][move->capture] &= ~(C64(1) << move->to);
			}
			t->piezas[c][move->piece] &= ~(C64(1) << move->from);
			t->piezas[c][move->piece] |= (C64(1) << move->to);

			if (c == blancas && move->piece == rey) {
				t->castlingRights &= ~WHITE_OO;
				t->castlingRights &= ~WHITE_OOO;
			} else if (c == negras && move->piece == rey) {
				t->castlingRights &= ~BLACK_OO;
				t->castlingRights &= ~BLACK_OOO;
			}

			if (move->capture == torre) {
				switch (move->to) {
					case h1: t->castlingRights &= ~WHITE_OO; break;
					case a1: t->castlingRights &= ~WHITE_OOO; break;
					case h8: t->castlingRights &= ~BLACK_OO; break;
					case a8: t->castlingRights &= ~BLACK_OOO; break;
					default: break;
				}
			}

			if (move->piece == torre) {
				casilla from = move->from;
				switch (from) {
					case h1: t->castlingRights &= ~WHITE_OO; break;
					case a1: t->castlingRights &= ~WHITE_OOO; break;
					case h8: t->castlingRights &= ~BLACK_OO; break;
					case a8: t->castlingRights &= ~BLACK_OOO; break;
					default: break;
				}
			}

			if (move->piece == peon && (abs(move->to - move->from) == 16)) {
				if (c == blancas) {
					t->enPassantSquare = move->to - 8;
				}
				if (c == negras) {
					t->enPassantSquare = move->to + 8;
				}
			}
			break;

		case 1:
			{
				casilla opponentPawn = (c == blancas ? enPassantSq - 8 : enPassantSq + 8);
				t->piezas[c][move->piece] &= ~(C64(1) << move->from);
				t->piezas[c][move->piece] |= (C64(1) << move->to);
				t->piezas[!c][peon] &= ~(C64(1) << opponentPawn);
			}
			break;

		case 2:
			t->piezas[c][move->piece] &= ~(C64(1) << move->from);
			t->piezas[c][move->piece] |= (C64(1) << move->to);

			switch (move->to) {
				case g1:
					if (BB_SQUARE(h1) & t->piezas[c][torre]) {
						t->piezas[c][torre] &= ~(C64(1) << h1);
						t->piezas[c][torre] |= (C64(1) << f1);
						t->castlingRights &= ~WHITE_OO;
						t->castlingRights &= ~WHITE_OOO;
					}
					break;
				case c1:
					if (BB_SQUARE(a1) & t->piezas[c][torre]) {
						t->piezas[c][torre] &= ~(C64(1) << a1);
						t->piezas[c][torre] |= (C64(1) << d1);
						t->castlingRights &= ~WHITE_OO;
						t->castlingRights &= ~WHITE_OOO;
					}
					break;
				case g8:
					if (BB_SQUARE(h8) & t->piezas[c][torre]) {
						t->piezas[c][torre] &= ~(C64(1) << h8);
						t->piezas[c][torre] |= (C64(1) << f8);
						t->castlingRights &= ~BLACK_OO;
						t->castlingRights &= ~BLACK_OOO;
					}
					break;
				case c8:
					if (BB_SQUARE(a8) & t->piezas[c][torre]) {
						t->piezas[c][torre] &= ~(C64(1) << a8);
						t->piezas[c][torre] |= (C64(1) << d8);
						t->castlingRights &= ~BLACK_OO;
						t->castlingRights &= ~BLACK_OOO;
					}
					break;
				default:
					break;
			}
			break;

		case 3:
			t->piezas[c][move->piece] &= ~(C64(1) << move->from);
			if (move->capture != 0) {
				t->piezas[!c][move->capture] &= ~(C64(1) << move->to);
			}
			t->piezas[c][move->piece] &= ~(C64(1) << move->to);
			t->piezas[c][move->promoPiece] |= (C64(1) << move->to);
			break;
	}
	updateBoardCache(t);
}

void unmakeMove(Move *move, Tablero *t, color c) {
	// Similar to makeMove but reversed
	// (Implementation would be the reverse operations)
	casilla enPassantSq = t->enPassantSquare;
	t->enPassantSquare = -1;
	t->fullMoves--;

	if (move->piece == peon) {
		t->halfmoveClock = 0;
	}

	switch (move->special) {
		case 0:
			if (move->capture != 0) {
				t->piezas[!c][move->capture] &= ~(C64(1) << move->to);
			}
			t->piezas[c][move->piece] &= ~(C64(1) << move->from);
			t->piezas[c][move->piece] |= (C64(1) << move->to);

			if (c == blancas && move->piece == rey) {
				t->castlingRights &= ~WHITE_OO;
				t->castlingRights &= ~WHITE_OOO;
			} else if (c == negras && move->piece == rey) {
				t->castlingRights &= ~BLACK_OO;
				t->castlingRights &= ~BLACK_OOO;
			}

			if (move->capture == torre) {
				switch (move->to) {
					case h1: t->castlingRights &= ~WHITE_OO; break;
					case a1: t->castlingRights &= ~WHITE_OOO; break;
					case h8: t->castlingRights &= ~BLACK_OO; break;
					case a8: t->castlingRights &= ~BLACK_OOO; break;
					default: break;
				}
			}

			if (move->piece == torre) {
				casilla from = move->from;
				switch (from) {
					case h1: t->castlingRights &= ~WHITE_OO; break;
					case a1: t->castlingRights &= ~WHITE_OOO; break;
					case h8: t->castlingRights &= ~BLACK_OO; break;
					case a8: t->castlingRights &= ~BLACK_OOO; break;
					default: break;
				}
			}

			if (move->piece == peon && (abs(move->to - move->from) == 16)) {
				if (c == blancas) {
					t->enPassantSquare = move->to - 8;
				}
				if (c == negras) {
					t->enPassantSquare = move->to + 8;
				}
			}
			break;

		case 1:
			{
				casilla opponentPawn = (c == blancas ? enPassantSq - 8 : enPassantSq + 8);
				t->piezas[c][move->piece] &= ~(C64(1) << move->from);
				t->piezas[c][move->piece] |= (C64(1) << move->to);
				t->piezas[!c][peon] &= ~(C64(1) << opponentPawn);
			}
			break;

		case 2:
			t->piezas[c][move->piece] &= ~(C64(1) << move->from);
			t->piezas[c][move->piece] |= (C64(1) << move->to);

			switch (move->to) {
				case g1:
					if (BB_SQUARE(h1) & t->piezas[c][torre]) {
						t->piezas[c][torre] &= ~(C64(1) << h1);
						t->piezas[c][torre] |= (C64(1) << f1);
						t->castlingRights &= ~WHITE_OO;
						t->castlingRights &= ~WHITE_OOO;
					}
					break;
				case c1:
					if (BB_SQUARE(a1) & t->piezas[c][torre]) {
						t->piezas[c][torre] &= ~(C64(1) << a1);
						t->piezas[c][torre] |= (C64(1) << d1);
						t->castlingRights &= ~WHITE_OO;
						t->castlingRights &= ~WHITE_OOO;
					}
					break;
				case g8:
					if (BB_SQUARE(h8) & t->piezas[c][torre]) {
						t->piezas[c][torre] &= ~(C64(1) << h8);
						t->piezas[c][torre] |= (C64(1) << f8);
						t->castlingRights &= ~BLACK_OO;
						t->castlingRights &= ~BLACK_OOO;
					}
					break;
				case c8:
					if (BB_SQUARE(a8) & t->piezas[c][torre]) {
						t->piezas[c][torre] &= ~(C64(1) << a8);
						t->piezas[c][torre] |= (C64(1) << d8);
						t->castlingRights &= ~BLACK_OO;
						t->castlingRights &= ~BLACK_OOO;
					}
					break;
				default:
					break;
			}
			break;

		case 3:
			t->piezas[c][move->piece] &= ~(C64(1) << move->from);
			if (move->capture != 0) {
				t->piezas[!c][move->capture] &= ~(C64(1) << move->to);
			}
			t->piezas[c][move->piece] &= ~(C64(1) << move->to);
			t->piezas[c][move->promoPiece] |= (C64(1) << move->to);
			break;
	}
	updateBoardCache(t);
}

tipoDePieza charToPiece(char c) {
	switch (c) {
		case 'p': case 'P': return peon;
		case 'r': case 'R': return torre;
		case 'n': case 'N': return caballo;
		case 'b': case 'B': return alfil;
		case 'q': case 'Q': return reina;
		case 'k': case 'K': return rey;
	}
	printf("error: invalid fen\n");
	return peon;
}

casilla stringToSq(const char *sq) {
	int file = tolower(sq[0]) - 'a';
	int rank = sq[1] - '1';
	return rank * 8 + file;
}

char pieceToChar(tipoDePieza piece) {
	switch (piece) {
		case peon: return 'p';
		case torre: return 'r';
		case caballo: return 'n';
		case alfil: return 'b';
		case reina: return 'q';
		case rey: return 'k';
	}
	printf("error: invalid fen\n");
	return ' ';
}

char *moveToStr(Move *move) {
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
