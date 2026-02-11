#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>
/*
 FIX:move genereation
 */
//importante
typedef uint64_t bitboard;
//macros
#define C64(constantU64) constantU64##ULL
#define BB_SQUARE(sq) (1ULL << (sq))
#define RANK(r) (C64(0xFF) << (8 * (r)))
//enums
typedef enum{
	blancas,negras
}color;
typedef enum{
	peon, caballo, alfil, torre, reina, rey
}tipoDePieza;
typedef enum {
	// Rank 1
	a1, b1, c1, d1, e1, f1, g1, h1,
	// Rank 2
	a2, b2, c2, d2, e2, f2, g2, h2,
	// Rank 3
	a3, b3, c3, d3, e3, f3, g3, h3,
	// Rank 4
	a4, b4, c4, d4, e4, f4, g4, h4,
	// Rank 5
	a5, b5, c5, d5, e5, f5, g5, h5,
	// Rank 6
	a6, b6, c6, d6, e6, f6, g6, h6,
	// Rank 7
	a7, b7, c7, d7, e7, f7, g7, h7,
	// Rank 8
	a8, b8, c8, d8, e8, f8, g8, h8
}casilla;
typedef struct{
	bitboard piezas[2][6];
	bitboard allPieces[2];
	bitboard allOccupiedSquares;
	int castlingRights[2]; // 0: no rooks or king move, 1: h: a rank rook already moved 2: h rank rook moved 3: king moved 4: all rooks moved 5: everything moved
}Tablero;
typedef struct{
	casilla from;
	casilla to;
	tipoDePieza piece;
	int capture;
	int special; // 0 normal, 1 en passant, 2 castling, 3 promo,
	int promoPiece; // 0 none
}Move;
typedef struct{
	Move moves[256];
	int count;
}moveLists;

//atack mascs
bitboard knightAttacks[64];
bitboard kingAttacks[64];
bitboard pawnAttacks[2][64];
//sliding pieces offsets
int rookOffsets[4] = {8,-8,-1,1}; // up, down, left, right
int bishopOffsets[4] = {-9,-7,7,9}; //up-left, up-right,down-left,down-right
int QueenOffsets[8] = {8,-8,-1,1,-9,-7,7,9};
// misc
Tablero tablero =  {
	.piezas = {{0}},
	.allPieces = {0},
	.allOccupiedSquares = 0,
	.castlingRights = {0,0}
};
//funciones
void printBitboard(bitboard bb);
void initBoard(Tablero * t);
void updateBoardCache(Tablero * t);
void generateAllMoves(color c, Tablero * t, moveLists * output);
bool isAttacked(Tablero *t, int square, color attackerColor);
bitboard computeKnightAttacks(casilla sq);
bitboard computeKingAttacks(casilla sq);
bitboard computePawnAttacks(color c,casilla sq);
void initAttackTables();
void generateKnightMoves(moveLists * ml, color c,Tablero * t);
void generateKingMoves(moveLists * ml, color c, Tablero * t);
void generatePawnMoves(moveLists * ml, color c, Tablero * t);
void generateRookMoves(moveLists * ml, color c, Tablero * t);
void generateBishopMoves(moveLists * ml, color c, Tablero * t);
void generateQueenMoves(moveLists * ml, color c, Tablero * t);
float boardEval(Tablero * t);

int main(){
	bool isPlaying = true;
	color currentColor = blancas;
	initBoard(&tablero);
	initAttackTables();
	printBitboard(tablero.allOccupiedSquares);
	moveLists myMoves, theirMoves;
	generateAllMoves(currentColor, &tablero, &myMoves);
	generateAllMoves(!currentColor, &tablero, &theirMoves);
}
//debug
void printBitboard(bitboard bb){
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
void initBoard(Tablero * t){
	memset(t,0,sizeof(Tablero));

	//blancas
	t->piezas[blancas][peon] = RANK(2 - 1);
	t->piezas[blancas][caballo] = BB_SQUARE(b1) | BB_SQUARE(g1);
	t->piezas[blancas][alfil] = BB_SQUARE(c1) | BB_SQUARE(f1);
	t->piezas[blancas][torre] = BB_SQUARE(a1) | BB_SQUARE(h1);
	t->piezas[blancas][reina] = BB_SQUARE(d1);
	t->piezas[blancas][rey] = BB_SQUARE(e1);

	//negras
	t->piezas[negras][peon] = RANK(7 - 1);
	t->piezas[negras][caballo] = BB_SQUARE(b8) | BB_SQUARE(g8); 
	t->piezas[negras][alfil] = BB_SQUARE(c8) | BB_SQUARE(f8);
	t->piezas[negras][torre] = BB_SQUARE(a8) | BB_SQUARE(h8);
	t->piezas[negras][reina] = BB_SQUARE(d8);
	t->piezas[negras][rey] = BB_SQUARE(e8);
	updateBoardCache(t);
}
void updateBoardCache(Tablero * t){
	t->allPieces[blancas] = t->piezas[blancas][peon]|
		t->piezas[blancas][caballo]|
		t->piezas[blancas][alfil]|
		t->piezas[blancas][torre]|
		t->piezas[blancas][reina]|
		t->piezas[blancas][rey];


	t->allPieces[negras] = t->piezas[negras][peon]|
		t->piezas[negras][caballo]|
		t->piezas[negras][alfil]|
		t->piezas[negras][torre]|
		t->piezas[negras][reina]|
		t->piezas[negras][rey];
	t->allOccupiedSquares = t->allPieces[blancas] | t->allPieces[negras];
}
void generateAllMoves(color c, Tablero * t, moveLists * output){
	output->count = 0;
	printf("=== Checking %s Knight moves ===\n", c == blancas ? "white" : "black");
	generateKnightMoves(output, c, t);
	printf("=== Knight moves done, %d moves added\n",output->count);
	int count = output->count;
	printf("=== Checking %s King moves ===\n", c == blancas ? "white" : "black");
	generateKingMoves(output, c, t);
	printf("=== King moves done, %d moves added\n",output->count - count);
	count = output->count;
	printf("=== Checking %s Pawn moves ===\n", c == blancas ? "white" : "black");
	generatePawnMoves(output, c, t);
	printf("=== Pawn moves done, %d moves added\n",output->count - count);
	count = output->count;
	printf("=== Checking %s Rook moves ===\n", c == blancas ? "white" : "black");
	generateRookMoves(output, c, t);
	printf("=== Rook moves done, %d moves added\n",output->count - count);
	count = output->count;
	printf("=== Checking  %s Bishop moves ===\n", c == blancas ? "white" : "black");
	generateBishopMoves(output, c, t);
	printf("=== Bishop moves done, %d moves added\n",output->count - count);
	count = output->count;
	printf("=== Checking %s Queen moves ===\n", c == blancas ? "white" : "black");
	generateQueenMoves(output, c, t);
	printf("=== Queen moves done, %d moves added\n",output->count - count);
	for(int i = 0; i < 4; i++){
		printf("\n");
	}
}
bool isAttacked(Tablero * t, int square, color attackerColor){
	bitboard king = t->piezas[attackerColor][rey];
	if(king){
		int from = __builtin_ctzll(king);
		bitboard sqAttackedByKing = kingAttacks[from];
		if(sqAttackedByKing & (1ULL << square)) return true;
	}
	moveLists opponentMoves = {0};
	generateRookMoves(&opponentMoves,attackerColor,t);
	generateKnightMoves(&opponentMoves,attackerColor,t);
	generateBishopMoves(&opponentMoves,attackerColor,t);
	generateQueenMoves(&opponentMoves,attackerColor,t);
	generatePawnMoves(&opponentMoves,attackerColor,t);
	for(int i = 0; i < opponentMoves.count; i++){
		if(opponentMoves.moves[i].to == square){
			return true;
		}
	}
	return false;
}

bitboard computeKnightAttacks(casilla sq){
	int rank = sq / 8;
	int file = sq % 8;
	bitboard result = 0;

	int moves[8][2] = {
		{2,1}, {2,-1}, {-2,1}, {-2,-1},
		{1,2}, {1,-2}, {-1,2}, {-1,-2}
	};
	for(int i = 0; i < 8; i++){
		int newRank = rank + moves[i][0];
		int newFile = file + moves[i][1];

		if(newRank >= 0 && newRank < 8 && newFile >= 0 && newFile < 8){
			int destSquare = newRank * 8 + newFile;
			result |= BB_SQUARE(destSquare);
		}
	}
	return result;
}
bitboard computeKingAttacks(casilla sq){
	int rank = sq / 8;
	int file = sq % 8;
	bitboard result = 0;

	int moves[8][2] = {
		{1,0}, {1,1}, {0,1},{-1,1},{-1,0},{-1,-1},{0,-1},{1,-1}
	};
	for(int i = 0; i < 8; i++){
		int newRank = rank + moves[i][0];
		int newFile = file + moves[i][1];
		if(newRank >= 0 && newRank < 8 && newFile >= 0 && newFile < 8){
			int destSquare = newRank * 8 + newFile;
			result |= BB_SQUARE(destSquare);
		}
	}
	return result;
}
bitboard computePawnAttacks(color c, casilla sq){
	int rank = sq / 8;
	int file = sq % 8;
	bitboard result = 0;
	int moves[2][2][2] = {
		{{1,-1}, {1,1}},
		{{-1,-1}, {-1,1}}
	};
	int color = (c == blancas) ? 0 : 1;

	for(int i = 0; i < 2;i++){
		int newRank = rank + moves[color][i][0];
		int newFile = file + moves[color][i][1];

		if(newRank >= 0 && newRank < 8 && newFile >= 0 && newFile < 8){
			int destSquare = newRank * 8 + newFile;
			result |= BB_SQUARE(destSquare);
		}
	}

	return result;
}
void initAttackTables(){
	//caballo
	for(int i = 0; i < 64; i++){
		knightAttacks[i] = computeKnightAttacks(i);
	}
	//rey
	for(int i = 0; i < 64; i++){
		kingAttacks[i] = computeKingAttacks(i);
	}
	//peon
	for(int i = 0; i < 64; i++){
		pawnAttacks[blancas][i] = computePawnAttacks(blancas,i);
		pawnAttacks[negras][i] = computePawnAttacks(negras,i);
	}
}
void generateKnightMoves(moveLists * ml, color c,Tablero * t){
	bitboard allKnights  = t->piezas[c][caballo];
	while(allKnights){
		casilla from = __builtin_ctzll(allKnights);
		allKnights &= (allKnights - 1);
		bitboard attacks = (knightAttacks[from] & (~t->allPieces[c]));
		while(attacks){
			casilla to = __builtin_ctzll(attacks);
			attacks &= (attacks - 1);
			int capture = 0;
			if(BB_SQUARE(to) & t->allPieces[!c]){
				for(int piece = peon; piece <= rey; piece++){
					if(t->piezas[!c][piece] & BB_SQUARE(to)){
						capture = piece;
						break;
					}
				}
			}
			Move move = {from,to,caballo,capture,0,0};
			ml->moves[ml->count] = move;
			ml->count++;
		}
	}

}
void generateKingMoves(moveLists * ml, color c, Tablero * t){
	bitboard king = t->piezas[c][rey];
	if(king == 0){
		// this should NOT happen
		return;
	}
	if(king & (king - 1)){
		return;
	}
	casilla from = __builtin_ctzll(king);
	bitboard attacks = kingAttacks[from] & (~t->allPieces[c]);
	for(int i = 0; i < 64; i++){
		if(attacks & (C64(1) << i)){
			int capture = 0;
			if(BB_SQUARE(i) & t->allPieces[!c]){
				for(int piece = peon; piece <= rey; piece++){
					if(t->piezas[!c][piece] & BB_SQUARE(i) && !(isAttacked(t,i,!c))){
						capture = piece;
						break;
					}
				}
				Move move = {from,i,rey,capture,0,0};
				ml->moves[ml->count] = move;
				ml->count++;
			}
			else if(!(isAttacked(t,i,!c))){
				Move move = {from,i,rey,capture,0,0};
				ml->moves[ml->count] = move;
				ml->count++;
			}

		}
	}
	//castling ROOKS IS HANDLED WHEN MAKING MOVE
	// 0: no rooks or king move, 1: h: a rank rook already moved 2: h rank rook moved 3: king moved 4: all rooks moved 5: everything moved
	if(c == blancas && from == e1){
		bitboard rooks = t->piezas[c][torre];
		while(rooks){
			bool canCastleQueenSide = true;
			bool canCastleKingSide = true;
			casilla rook = __builtin_ctzll(rooks);
			rooks &= (rooks - 1);
			if(rook == h1 && (t->castlingRights[c] == 0 || t->castlingRights[c] == 1)){
				for(casilla sq = f1; sq < h1; sq++){
					if((BB_SQUARE(sq) & (t->allPieces[c] | t->allPieces[!c]))){
						canCastleKingSide = false;
					}
				}
				if(canCastleKingSide && !(isAttacked(t,g1,!c))){
					casilla to = g1;
					Move move = {from,to,rey,0,2,0};
					ml->moves[ml->count] = move;
					ml->count++;
				}
			}
			else if(rook == a1 && (t->castlingRights[c] == 0 || t->castlingRights[c] == 2)){
				for(casilla sq = d1; sq > a1;sq--){
					if(BB_SQUARE(sq) & (t->allPieces[c] | t->allPieces[!c])){
						canCastleQueenSide = false;
					}
				}
				if(canCastleQueenSide && !(isAttacked(t,c1,!c))){
					casilla to = c1;
					Move move = {from,to,rey,0,2,0};
					ml->moves[ml->count] = move;
					ml->count++;
				}
			}
			else{
				printf("error on castling move generation, prolly an invalid square: %d\n",rook);
			}
		}
	}
	if(c == negras && from == e8){
		bitboard rooks = t->piezas[c][torre];
		while(rooks){
			casilla rook = __builtin_ctzll(rooks);
			rooks &= (rooks - 1);
			bool canCastleQueenSide = true;
			bool canCastleKingSide = true;
			if(rook == h8 && (t->castlingRights[c] == 0 || t->castlingRights[c] == 1)){
				for(casilla sq = f8; sq < h8; sq++){
					if(BB_SQUARE(sq) & (t->allPieces[c] | t->allPieces[!c])){
						canCastleKingSide = false;
					}
				}

				if(canCastleKingSide){
					casilla to = g8;
					Move move = {from,to,rey,0,2,0};
					ml->moves[ml->count] = move;
					ml->count++;
				}
			}
			else if(rook == a8 && (t->castlingRights[c] == 0 || t->castlingRights[c] == 2)){
				for(casilla sq = d8; sq > a8; sq--){
					if(BB_SQUARE(sq) & (t->allPieces[c] | t->allPieces[!c])){
						canCastleQueenSide = false;
					}
				}
				if(canCastleQueenSide){
					casilla to = c8;
					Move move = {from,to,rey,0,2,0};
					ml->moves[ml->count] = move;
					ml->count++;
				}
			}
			else{
				printf("error on castling move generation, prolly an invalid square: %d\n",rook);
			}
		}
	}
}
void generatePawnMoves(moveLists * ml, color c, Tablero * t){
	bitboard allPawns = t->piezas[c][peon];
	/*
	TODO: En passant
	*/
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
						Move move = {from,to,peon,3,i};
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
			pawnAttacksLocal &= t->allPieces[!c];
			while(pawnAttacksLocal){
				int capture = 0;
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
			}
			else if(c == negras && !(t->allOccupiedSquares & BB_SQUARE(from - 8)) &&
				!(t->allOccupiedSquares & BB_SQUARE(from - 16))){
				to = from - 16;
			}
			Move move = {from,to,peon,0,0};
			ml->moves[ml->count] = move;
			ml->count++;
		}
	}
}
void generateRookMoves(moveLists * ml, color c, Tablero * t){
	bitboard allRooks = (t->piezas[c][torre]);
	while(allRooks){
		casilla from = __builtin_ctzll(allRooks);
		allRooks &= (allRooks - 1);
		for(int i = 0; i < 4; i++){
			int offset = rookOffsets[i];
			int j = 1;
			casilla to = from + (j * offset);
			while((to >= 0) && (to < 64) && (to / 8 == from / 8 || to % 8 == from % 8)){ // check if we are moving by the same factor vertically and horizontallly, if true not enter loop
				int capture = 0;
				if(BB_SQUARE(to) & t->allPieces[!c]){
					for(int piece =  peon; piece <= rey; piece++){
						if(t->piezas[!c][piece] & BB_SQUARE(to)){
							capture = piece;
							Move move = {from,to,torre,capture,0,0};
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
					Move move = {from,to, torre, capture,0,0};
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
void generateQueenMoves(moveLists * ml, color c, Tablero * t){
	bitboard allQueens = t->piezas[c][reina];
	while(allQueens){
		casilla from = __builtin_ctzll(allQueens);
		allQueens &= (allQueens - 1);
		for(int i = 0; i < 8; i++){
			int offset = QueenOffsets[i];
			int j = 1;
			casilla to = from + (j * offset);
			if(i < 4){
				while((to >= 0) && (to < 64) && (to / 8 == from / 8 || to % 8 == from % 8)){
					int capture = 0;
					if(BB_SQUARE(to) & t->allPieces[!c]){
						for(int piece =  peon; piece <= rey; piece++){
							if(t->piezas[!c][piece] & BB_SQUARE(to)){
								capture = piece;
								Move move = {from,to,reina,capture,0,0};
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
						Move move = {from,to, reina, capture,0,0};
						ml->moves[ml->count] = move;
						ml->count++;
					}
					j++;
					to = from + (j * offset);
				}
      			}
			else if(i >= 4){
				while(to >= 0 && to < 64 && abs((from % 8) - (to % 8)) == abs((from / 8) - (to / 8))){
					int capture = 0;
					if(BB_SQUARE(to) & t->allPieces[!c]){
						for(int piece =  peon; piece <= rey; piece++){
							if(t->piezas[!c][piece] & BB_SQUARE(to)){
								capture = piece;
								Move move = {from,to,reina,capture,0,0};
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
						Move move = {from,to, reina, capture,0,0};
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
float boardEval(Tablero * t){
	// queen = 9; rook = 5; bishop = 3; knight = 3; pawn = 1;
	moveLists white;
	generateAllMoves(blancas,t,&white);
	moveLists black;
	generateAllMoves(negras,t,&black);
	float value =
	200 * (t->piezas[blancas][rey] - t->piezas[negras][rey]) +
	1 * (t->piezas[blancas][peon] - t->piezas[negras][peon]) +
	3 * (t->piezas[blancas][caballo] - t->piezas[negras][caballo] + (t->piezas[blancas][alfil] - t->piezas[negras][alfil])) +
	5 * (t->piezas[blancas][torre] - t->piezas[negras][torre]) +
	9 * (t->piezas[blancas][reina] - t->piezas[negras][reina]) +
	0.1 * (white.count - black.count);
	float scaledValue = tanh(value / 25.0f); 
	return scaledValue;
}
