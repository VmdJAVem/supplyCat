#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
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
	// Rank 2  a2, b2, c2, d2, e2, f2, g2, h2, Rank 3 a3, b3, c3, d3, e3, f3, g3, h3, Rank 4 a4, b4, c4, d4, e4, f4, g4, h4,
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
}Tablero;
typedef struct{
	casilla from;
	casilla to;
	tipoDePieza piece;
	int capture;
	int special; // 0 normal, 1 en passant, 2 castling, 3 promo,
}Move;
typedef struct{
	Move moves[256];
	int count;
}moveLists;

//atack mascs
bitboard knightAttacks[64];
bitboard kingAttacks[64];
bitboard pawnAttacks[2][64];

// misc
Tablero tablero;

//funciones
void printBitboard(bitboard bb);
void initBoard(Tablero * t);
void updateBoardCache(Tablero * t);
bitboard computeKnightAttacks(casilla sq);
bitboard computeKingAttacks(casilla sq);
bitboard computePawnAttacks(color c,casilla sq);
void initAttackTables();
void generateKnightMoves(moveLists * ml, color c,Tablero * t);
void generateKingMoves(moveLists * ml, color c, Tablero * t);

int main(){
	initBoard(&tablero);
	printBitboard(tablero.allOccupiedSquares);
	initAttackTables();
	printf("Knight's Attack:\n");
	for(int i = 0;i < 64;i++){
		printBitboard(knightAttacks[i]);
		sleep(1);
	}
	sleep(5);
	printf("Rey:\n");
	for(int i = 0;i < 64;i++){
		printBitboard(kingAttacks[i]);
		sleep(1);
	}
	sleep(5);
	printf("peon blanco:\n");
	for(int j = 0;j < 64;j++){
		printBitboard(pawnAttacks[blancas][j]);
		sleep(1);
	}
	sleep(5);
	printf("peon negro\n");
	for(int j = 0;j < 64;j++){
		printBitboard(pawnAttacks[negras][j]);
		sleep(1);
	}
	return 0;
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
	for(int i = 0; i < 64; i++){
		if(allKnights  & (C64(1) << i)){
			bitboard pseudoMoves = knightAttacks[i];
			pseudoMoves &= (~t->allPieces[c]);
			for(int j = 0; j < 64; j++){
				if(pseudoMoves & (C64(1) << j)){
					int capture = 0;
					if(BB_SQUARE(j) & t->allPieces[!c]){
						for(int piece = peon; piece <= rey; piece++){
							if(t->piezas[!c][piece] & BB_SQUARE(j)){
								capture = piece;
								break;
							}
						}
					}
					Move move = {i,j,caballo,capture,0};
					ml->moves[ml->count] = move;
					ml->count++;
				}
			}
		}
	}
}
void generateKingMoves(moveLists * ml, color c, Tablero * t){
	//TODO: castling
	bitboard king = t->piezas[c][rey];
	if(king == 0){
		// this should NOT happen
		return;
	}
	int from = __builtin_ctzll(king);

	bitboard attacks = kingAttacks[from] & (~t->allPieces[c]);
	for(int i = 0; i < 64; i++){
		if(attacks & (C64(1) << i)){
			int capture = 0;
			if(BB_SQUARE(i) & t->allPieces[!c]){
				for(int piece = peon; piece <= rey; piece++){
					if(t->piezas[!c][piece] & BB_SQUARE(i)){
						capture = piece;
						break;
					}
				}
				Move move = {from,i,rey,capture,0};
				ml->moves[ml->count] = move;
				ml->count++;
			}
		}
	}
}
