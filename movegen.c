#define _POSIX_C_SOURCE 199309L
#include "movegen.h"
#include "attack.h"
#include "board.h"
#include <stdio.h>
#include <string.h>
#include "move.h"
#include <math.h>
#include <stdlib.h>

void generateAllMoves(color c, Tablero *t, moveLists *output) {
	output->count = 0;
	moveLists temp = {0};

	generateKingMoves(&temp, c, t);
	generateKnightMoves(&temp, c, t);
	generatePawnMoves(&temp, c, t);
	generateRookMoves(&temp, c, t);
	generateBishopMoves(&temp, c, t);
	generateQueenMoves(&temp, c, t);

	for (int i = 0; i < temp.count; i++) {
		Tablero tempT = *t;
		makeMove(&temp.moves[i], &tempT, c);
		casilla king = __builtin_ctzll(tempT.piezas[c][rey]);
		if (!isAttacked(&tempT, king, !c)) {
			output->moves[output->count] = temp.moves[i];
			output->count++;
		}
	}
}

void generateKnightMoves(moveLists *ml, color c, Tablero *t) {
	bitboard allKnights = t->piezas[c][caballo];
	while (allKnights) {
		casilla from = __builtin_ctzll(allKnights);
		allKnights &= (allKnights - 1);
		bitboard attacks = (knightAttacks[from] & (~t->allPieces[c]));

		while (attacks) {
			casilla to = __builtin_ctzll(attacks);
			attacks &= (attacks - 1);
			int capture = 0;

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

void generateKingMoves(moveLists *ml, color c, Tablero *t) {
	bitboard king = t->piezas[c][rey];
	if (king == 0 || (king & (king - 1))) {
		return;
	}

	casilla from = __builtin_ctzll(king);
	bitboard attacks = kingAttacks[from] & (~t->allPieces[c]);

	for (int i = 0; i < 64; i++) {
		if (attacks & (C64(1) << i)) {
			int capture = 0;
			if (BB_SQUARE(i) & t->allPieces[!c]) {
				for (int piece = peon; piece <= rey; piece++) {
					if (t->piezas[!c][piece] & BB_SQUARE(i) && !(isAttacked(t, i, !c))) {
						capture = piece;
						break;
					}
				}
				Move move = {from, i, rey, capture, 0, 0};
				ml->moves[ml->count] = move;
				ml->count++;
			} else if (!(isAttacked(t, i, !c))) {
				Move move = {from, i, rey, capture, 0, 0};
				ml->moves[ml->count] = move;
				ml->count++;
			}
		}
	}

	// Castling logic
	if (c == blancas && from == e1) {
		bitboard rooks = t->piezas[c][torre];
		while (rooks) {
			bool canCastleQueenSide = true;
			bool canCastleKingSide = true;
			casilla rook = __builtin_ctzll(rooks);
			rooks &= (rooks - 1);

			if (rook == h1 && (t->castlingRights & WHITE_OO)) {
				for (casilla sq = f1; sq < h1; sq++) {
					if ((BB_SQUARE(sq) & (t->allPieces[c] | t->allPieces[!c])) || isAttacked(t, sq, !c)) {
						canCastleKingSide = false;
					}
				}
				if (canCastleKingSide && !(isAttacked(t, g1, !c) || isAttacked(t, from, !c))) {
					Move move = {from, g1, rey, 0, 2, 0};
					ml->moves[ml->count] = move;
					ml->count++;
				}
			} else if (rook == a1 && (t->castlingRights & WHITE_OOO)) {
				for (casilla sq = d1; sq > a1; sq--) {
					if ((BB_SQUARE(sq) & (t->allPieces[c] | t->allPieces[!c])) || isAttacked(t, sq, !c)) {
						canCastleQueenSide = false;
					}
				}
				if (canCastleQueenSide && !(isAttacked(t, c1, !c) || isAttacked(t, from, !c))) {
					Move move = {from, c1, rey, 0, 2, 0};
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
					if ((BB_SQUARE(sq) & (t->allPieces[c] | t->allPieces[!c])) || isAttacked(t, sq, !c)) {
						canCastleKingSide = false;
					}
				}
				if (canCastleKingSide && !(isAttacked(t, from, !c) || isAttacked(t, g8, !c))) {
					Move move = {from, g8, rey, 0, 2, 0};
					ml->moves[ml->count] = move;
					ml->count++;
				}
			} else if (rook == a8 && (t->castlingRights & BLACK_OOO)) {
				for (casilla sq = d8; sq > a8; sq--) {
					if ((BB_SQUARE(sq) & (t->allPieces[c] | t->allPieces[!c])) || isAttacked(t, sq, !c)) {
						canCastleQueenSide = false;
					}
				}
				if (canCastleQueenSide && !(isAttacked(t, from, !c) || isAttacked(t, c8, !c))) {
					Move move = {from, c8, rey, 0, 2, 0};
					ml->moves[ml->count] = move;
					ml->count++;
				}
			}
		}
	}
}
void generatePawnMoves(moveLists * ml, color c, Tablero * t){
	bitboard allPawns = t->piezas[c][peon];
	while(allPawns){
		casilla from = __builtin_ctzll(allPawns);
		allPawns &= (allPawns - 1);
		casilla to = 0;
		casilla rank = from / 8;
		bitboard pawnAttacksLocal = pawnAttacks[c][from];
		if(c == blancas){
			to =  from + 8;
		}
		else if(c == negras){
			to = from - 8;
		}
		if(to < 64 && to >= 0){
			if(BB_SQUARE(to) & (~t->allOccupiedSquares)){
				if((to / 8 == 1 && c == negras ) || (to / 8 == 6 && c == blancas)){
					for(int i = caballo; i < rey; i++){
						Move move = {from,to,peon,0,3,i};
						ml->moves[ml->count] = move;
						ml->count++;
					}
				}
				else{
					Move move = {from,to,peon,0,0,0};
					ml->moves[ml->count] = move;
					ml->count++;
				}
			}
		}
		if(pawnAttacksLocal & t->allPieces[!c]){
			pawnAttacksLocal &= t->allPieces[!c]; while(pawnAttacksLocal){ int capture = 0;
				if(pawnAttacksLocal & t->allPieces[!c]){
					to = __builtin_ctzll(pawnAttacksLocal);
					pawnAttacksLocal &= (pawnAttacksLocal - 1);
					for(int piece = peon; piece <= rey; piece++){
						if(t->piezas[!c][piece] & BB_SQUARE(to)){
							capture = piece;
							break;
						}
					}
					Move move = {from,to,peon,capture,0,0};
					ml->moves[ml->count] = move;
					ml->count++;
				}
			}
		}
		if((rank == 1) || (rank == 6)){
			if(c == blancas && !(t->allOccupiedSquares & BB_SQUARE(from + 8)) &&
				!(t->allOccupiedSquares & BB_SQUARE(from + 16))){
				to = from + 16;
				Move move = {from,to,peon,0,0,0};
				ml->moves[ml->count] = move;
				ml->count++;
			}
			else if(c == negras && !(t->allOccupiedSquares & BB_SQUARE(from - 8)) &&
				!(t->allOccupiedSquares & BB_SQUARE(from - 16))){
				to = from - 16;
				Move move = {from,to,peon,0,0,0};
				ml->moves[ml->count] = move;
				ml->count++;
			}

		}
		if(rank == 4 && t->enPassantSquare != 0 && c == blancas){
			if(t->enPassantSquare == (from + 7)){ // left
				to = from + 7;
				Move move = {from,to,peon,0,1,0};
				ml->moves[ml->count] = move;
				ml->count++;
			}
			if(t->enPassantSquare == (from  + 9)){ // right
				to = from + 9;
				Move move = {from,to,peon,0,1,0};
				ml->moves[ml->count] = move;
				ml->count++;

			}
		}
		else if(rank == 3 && t->enPassantSquare != 0 && c == negras){
			if(t->enPassantSquare == (from - 7)){ // left
				to = from - 7;
				Move move = {from,to,peon,0,1,0};
				ml->moves[ml->count] = move;
				ml->count++;
			}
			if(t->enPassantSquare == (from - 9)){ // right
				to = from - 9;
				Move move = {from,to,peon,0,1,0};
				ml->moves[ml->count] = move;
				ml->count++;

			}
		}
	}
}
void generateRookMoves(moveLists *ml, color c, Tablero *t) {
	bitboard allRooks = t->piezas[c][torre];
	while (allRooks) {
		casilla from = __builtin_ctzll(allRooks);
		allRooks &= (allRooks - 1);

		for (int i = 0; i < 4; i++) {
			int offset = rookOffsets[i];
			int j = 1;
			casilla to = from + (j * offset);

			while ((to >= 0) && (to < 64) && (to / 8 == from / 8 || to % 8 == from % 8)) {
				int capture = 0;
				if (BB_SQUARE(to) & t->allPieces[!c]) {
					for (int piece = peon; piece <= rey; piece++) {
						if (t->piezas[!c][piece] & BB_SQUARE(to)) {
							capture = piece;
							Move move = {from, to, torre, capture, 0, 0};
							ml->moves[ml->count] = move;
							ml->count++;
							break;
						}
					}
					break;
				} else if (BB_SQUARE(to) & t->allPieces[c]) {
					break;
				} else {
					Move move = {from, to, torre, capture, 0, 0};
					ml->moves[ml->count] = move;
					ml->count++;
				}
				j++;
				to = from + (j * offset);
			}
		}
	}
}
void generateBishopMoves(moveLists * ml, color c, Tablero * t){
	bitboard allBishops = t->piezas[c][alfil];
	while(allBishops){
		casilla from = __builtin_ctzll(allBishops);
		allBishops &= (allBishops - 1);
		for(int i = 0; i < 4; i++){
			int offset = bishopOffsets[i];
			int j = 1;
			casilla to = from + (j * offset);
			while(to >= 0 && to < 64 && abs((from % 8) - (to % 8)) == abs((from / 8) - (to / 8))){
				int capture = 0;
				if(BB_SQUARE(to) & t->allPieces[!c]){
					for(int piece =  peon; piece <= rey; piece++){
						if(t->piezas[!c][piece] & BB_SQUARE(to)){
							capture = piece;
							Move move = {from,to,alfil,capture,0,0};
							ml->moves[ml->count] = move;
							ml->count++;
							break;
						}
					}
					break;
				}
				else if(BB_SQUARE(to) & t->allPieces[c]){
					break;
				}
				else{
					Move move = {from,to, alfil, capture,0,0};
					ml->moves[ml->count] = move;
					ml->count++;
				}
				j++;
				to = from + (j * offset);
			}
		}
	}
}
void generateQueenMoves(moveLists *ml, color c, Tablero *t) {
	bitboard allQueens = t->piezas[c][reina];
	while (allQueens) {
		casilla from = __builtin_ctzll(allQueens);
		allQueens &= (allQueens - 1);

		for (int i = 0; i < 8; i++) {
			int offset = queenOffsets[i];
			int j = 1;
			casilla to = from + (j * offset);

			if (i < 4) {
				while ((to >= 0) && (to < 64) && (to / 8 == from / 8 || to % 8 == from % 8)) {
					int capture = 0;
					if (BB_SQUARE(to) & t->allPieces[!c]) {
						for (int piece = peon; piece <= rey; piece++) {
							if (t->piezas[!c][piece] & BB_SQUARE(to)) {
								capture = piece;
								Move move = {from, to, reina, capture, 0, 0};
								ml->moves[ml->count] = move;
								ml->count++;
								break;
							}
						}
						break;
					} else if (BB_SQUARE(to) & t->allPieces[c]) {
						break;
					} else {
						Move move = {from, to, reina, capture, 0, 0};
						ml->moves[ml->count] = move;
						ml->count++;
					}
					j++;
					to = from + (j * offset);
				}
			} else {
				while (to >= 0 && to < 64 && abs((from % 8) - (to % 8)) == abs((from / 8) - (to / 8))) {
					int capture = 0;
					if (BB_SQUARE(to) & t->allPieces[!c]) {
						for (int piece = peon; piece <= rey; piece++) {
							if (t->piezas[!c][piece] & BB_SQUARE(to)) {
								capture = piece;
								Move move = {from, to, reina, capture, 0, 0};
								ml->moves[ml->count] = move;
								ml->count++;
								break;
							}
						}
						break;
					} else if (BB_SQUARE(to) & t->allPieces[c]) {
						break;
					} else {
						Move move = {from, to, reina, capture, 0, 0};
						ml->moves[ml->count] = move;
						ml->count++;
					}
					j++;
					to = from + (j * offset);
				}
			}
		}
	}
}
