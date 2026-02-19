#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <sys/select.h>
#include <ctype.h>
//importante
typedef uint64_t bitboard;
//macros
#define C64(constantU64) constantU64##ULL
#define BB_SQUARE(sq) (1ULL << (sq))
#define RANK(r) (C64(0xFF) << (8 * (r)))
#define C8 ( (uint8_t)123 )
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
	uint8_t castlingRights;
	casilla enPassantSquare;
	int halfmoveClock;
	int fullMoves;
}Tablero;
const uint8_t WHITE_OO = 1;
const uint8_t WHITE_OOO = 2;
const uint8_t BLACK_OO = 4;
const uint8_t BLACK_OOO = 8;

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
typedef struct{
	Move move;
	float score;
} moveScore;

//atack mascs
bitboard knightAttacks[64];
bitboard kingAttacks[64];
bitboard pawnAttacks[2][64];
//sliding pieces offsets
int rookOffsets[4] = {8,-8,-1,1}; // up, down, left, right
int bishopOffsets[4] = {-9,-7,7,9}; //up-left, up-right,down-left,down-right
int QueenOffsets[8] = {8,-8,-1,1,-9,-7,7,9};
// misc
Tablero tablero =  {0};
bool isChecked[2] = {false};
volatile bool stopRequested = false;
bool isPlaying = true;
color colorToMove;
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
float boardEval(Tablero * t, color c);
void makeMove(Move * move, Tablero * t, color c);
moveScore negaMax(int depth, Tablero * t, color c);
float recursiveNegaMax(int depth, Tablero * t, color c, float alpha, float beta);
void proccesUCICommands(char command[256], Tablero * t);
bool inputAvaliable();
tipoDePieza charToPiece(char c);
void proccesUCICommands(char command[256], Tablero * t);
casilla stringToSq(const char * sq);

int main(){
	Tablero tablero =   {0};
	while(true){
		if(inputAvaliable()){
			char buffer[256];
			if(fgets(buffer, sizeof(buffer), stdin)){
				proccesUCICommands(buffer, &tablero);
			}
		}

	}

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
float recursiveNegaMax(int depth, Tablero * t, color c, float alpha, float beta){
	moveLists colorToMove = {0};
	generateAllMoves(c, t, &colorToMove);
	if(depth == 0 || colorToMove.count == 0){
		return boardEval(t, c);
	}

	for(int i = 0; i < colorToMove.count; i++){
		if(!stopRequested){
			Tablero temp = *t;
			makeMove(&colorToMove.moves[i], &temp, c);
			float score = -recursiveNegaMax(depth - 1, &temp, !c, -beta, -alpha);
			if(inputAvaliable()){
				char buffer[256];
				if(fgets(buffer, sizeof(buffer), stdin)){
					proccesUCICommands(buffer, t);
				}
			}

			if(score > alpha){
				alpha = score;
			}
			if(alpha >= beta){

				break;
			}
		}
	}
	return alpha;
}
moveScore negaMax(int depth, Tablero * t, color c){
	moveLists colorToMove = {0};
	generateAllMoves(c, t, &colorToMove);
	float bestScore = -INFINITY;
	Move bestMove = {0};
	float alpha = -INFINITY;
	float beta = +INFINITY;
	stopRequested = false;
	if(depth == 0 || colorToMove.count == 0){
		moveScore output = {
			.move = {0},
			.score = boardEval(t, c),
		};
		return output;
	}
	for(int i = 0; i < colorToMove.count; i++){
		if(!stopRequested){
			Tablero temp = *t;
			makeMove(&colorToMove.moves[i], &temp, c);

			float score = -recursiveNegaMax(depth - 1, &temp, !c, -beta, -alpha);
			if(inputAvaliable()){
				char buffer[256];
				if(fgets(buffer, sizeof(buffer), stdin)){
					proccesUCICommands(buffer, t);
				}
			}

			if(score > bestScore){
				bestScore = score;
				bestMove = colorToMove.moves[i];
			}
		}
	}
	moveScore output = {
		.move = bestMove,
		.score = bestScore
	};
	return output;
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
	t->castlingRights = WHITE_OO | WHITE_OOO | BLACK_OO | BLACK_OOO;
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
	moveLists temp = {0};
	generateKingMoves(&temp, c, t);
	generateKnightMoves(&temp, c, t);
	generatePawnMoves(&temp, c, t);
	generateRookMoves(&temp, c, t);
	generateBishopMoves(&temp, c, t),
		generateQueenMoves(&temp, c, t);
	for(int i = 0; i < temp.count; i++){
		Tablero tempT = *t;
		makeMove(&temp.moves[i], &tempT, c);
		casilla king = __builtin_ctzll(tempT.piezas[c][rey]);
		if(!isAttacked(&tempT, king, !c)){
			output->moves[output->count] = temp.moves[i];
			output->count++;
		}
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
	if(c == blancas && from == e1){
		bitboard rooks = t->piezas[c][torre];
		while(rooks){
			bool canCastleQueenSide = true;
			bool canCastleKingSide = true;
			casilla rook = __builtin_ctzll(rooks);
			rooks &= (rooks - 1);
			if(rook == h1 && (t->castlingRights & WHITE_OO)){
				for(casilla sq = f1; sq < h1; sq++){
					if((BB_SQUARE(sq) & (t->allPieces[c] | t->allPieces[!c])) || isAttacked(t,sq,!c)){
						canCastleKingSide = false;
					}
				}
				if(canCastleKingSide && !(isAttacked(t,g1,!c) || isAttacked(t,from,!c))){ // is the square we are moving to attacked || is the king in check
					casilla to = g1;
					Move move = {from,to,rey,0,2,0};
					ml->moves[ml->count] = move;
					ml->count++;
				}
			}
			else if(rook == a1 && (t->castlingRights & WHITE_OOO)){
				for(casilla sq = d1; sq > a1;sq--){
					if((BB_SQUARE(sq) & (t->allPieces[c] | t->allPieces[!c])) || isAttacked(t,sq,!c)){
						canCastleQueenSide = false;
					}
				}
				if(canCastleQueenSide && !(isAttacked(t,c1,!c) || isAttacked(t,from,!c))){
					casilla to = c1;
					Move move = {from,to,rey,0,2,0};
					ml->moves[ml->count] = move;
					ml->count++;
				}
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
			if(rook == h8 && (t->castlingRights & BLACK_OO)){
				for(casilla sq = f8; sq < h8; sq++){
					if((BB_SQUARE(sq) & (t->allPieces[c] | t->allPieces[!c])) || isAttacked(t,sq,!c)){
						canCastleKingSide = false;
					}
				}

				if(canCastleKingSide && !(isAttacked(t,from,!c) || isAttacked(t,g8, !c))){
					casilla to = g8;
					Move move = {from,to,rey,0,2,0};
					ml->moves[ml->count] = move;
					ml->count++;
				}
			}
			else if(rook == a8 && (t->castlingRights & BLACK_OOO)){
				for(casilla sq = d8; sq > a8; sq--){
					if((BB_SQUARE(sq) & (t->allPieces[c] | t->allPieces[!c])) || isAttacked(t,sq,!c)){
						canCastleQueenSide = false;
					}
				}
				if(canCastleQueenSide && !(isAttacked(t,from,!c) || isAttacked(t,c8,!c))){
					casilla to = c8;
					Move move = {from,to,rey,0,2,0};
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
float boardEval(Tablero * t, color c){
	// queen = 9; rook = 5; bishop = 3; knight = 3; pawn = 1;
	casilla king = __builtin_ctzll(t->piezas[c][rey]);
	moveLists movePerColor[2] = {0};
	moveLists possibleMoves = {0};
	generateAllMoves(c,t,&possibleMoves);

	if(possibleMoves.count == 0){
		if(isAttacked(t,king,!c)){
			return -1.0f;
		}
		else if(possibleMoves.count == 0){
			return 0.0f;
		}
	}
	movePerColor[c] = possibleMoves;
	generateAllMoves(!c,t,&movePerColor[!c]);


	float value =
		1 * (__builtin_popcountll(t->piezas[c][peon]) - __builtin_popcountll(t->piezas[!c][peon])) +
		3 * ((__builtin_popcountll(t->piezas[c][caballo]) - __builtin_popcountll(t->piezas[!c][caballo])) + (__builtin_popcountll(t->piezas[c][alfil]) - __builtin_popcountll(t->piezas[!c][alfil]))) +
		5 * (__builtin_popcountll(t->piezas[c][torre]) - __builtin_popcountll(t->piezas[!c][torre])) +
		9 * (__builtin_popcountll(t->piezas[c][reina]) - __builtin_popcountll(t->piezas[!c][reina])) +
		0.1 * (movePerColor[c].count - movePerColor[!c].count);
	float scaledValue = tanh(value / 25.0f);
	return scaledValue;
}

void makeMove(Move * move, Tablero * t, color c){
	casilla enPassantSq = t-> enPassantSquare;
	t->enPassantSquare = -1;
	t->fullMoves++;
	if(move->piece == peon){
		t->halfmoveClock = 0;
	}
	switch(move->special){
		case 0:
			if(move->capture != 0){
				t->piezas[!c][move->capture] &= ~(C64(1) << move->to);
			}
			t->piezas[c][move->piece] &= ~(C64(1) << move->from);
			t->piezas[c][move->piece] |= (C64(1) << move->to);
			if(c == blancas && move->piece == rey){
				t->castlingRights &= ~WHITE_OO;
				t->castlingRights &= ~WHITE_OOO;

			}
			else if(c == negras && move->piece == rey){
				t->castlingRights &= ~BLACK_OO;
				t->castlingRights &=  ~BLACK_OOO;
			}
			if(move->capture == torre){
				switch(move->to){
					case h1: t->castlingRights &= ~WHITE_OO; break;
					case a1: t->castlingRights &= ~WHITE_OOO; break;
					case h8: t->castlingRights &= ~BLACK_OO; break;
					case a8: t->castlingRights &= ~BLACK_OOO; break;
					default: break;
				}
			}
			if(move->piece == torre){
				casilla from = move->from;
				switch(from){
					case h1: t->castlingRights &= ~WHITE_OO; break;
					case a1: t->castlingRights &= ~WHITE_OOO; break;
					case h8: t->castlingRights &= ~BLACK_OO; break;
					case a8: t->castlingRights &= ~BLACK_OOO; break; 
					default: break;
				}
			}
			if(move->piece == peon && (abs(move->to - move->from) == 16)){
				if(c == blancas){
					t->enPassantSquare = move->to - 8;
				}
				if(c == negras){
					t->enPassantSquare = move->to + 8;
				}
			}
			break;
		case 1:
			casilla opponentPawn = (c == blancas ? enPassantSq - 8 : enPassantSq + 8);
			t->piezas[c][move->piece] &= ~(C64(1) << move->from);
			t->piezas[c][move->piece] |= (C64(1) << move->to);
			t->piezas[!c][peon] &= ~(C64(1) << opponentPawn);
			break;
		case 2:
			//castling
			t->piezas[c][move->piece] &= ~(C64(1) << move->from);
			t->piezas[c][move->piece] |= (C64(1) << move->to);
			switch(move->to){
				case g1:
					if(BB_SQUARE(h1) & t->piezas[c][torre]){
						t->piezas[c][torre] &= ~(C64(1) << h1);
						t->piezas[c][torre] |= (C64(1) << f1);
						t->castlingRights &= ~WHITE_OO;
						t->castlingRights &= ~WHITE_OOO;
					}
					break;
				case c1:
					if(BB_SQUARE(a1) & t->piezas[c][torre]){
						t->piezas[c][torre] &= ~(C64(1) << a1);
						t->piezas[c][torre] |= (C64(1) << d1);
						t->castlingRights &= ~WHITE_OO;
						t->castlingRights &= ~WHITE_OOO;
					}
					break;
				case g8:
					if(BB_SQUARE(h8) & t->piezas[c][torre]){
						t->piezas[c][torre] &= ~(C64(1) << h8);
						t->piezas[c][torre] |= (C64(1) << f8);
						t->castlingRights &= ~BLACK_OO;
						t->castlingRights &= ~BLACK_OOO;
					}
					break;
				case c8:
					if(BB_SQUARE(a8) & t->piezas[c][torre]){
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
			//promotion
			t->piezas[c][move->piece] &= ~(C64(1) << move->from);
			if(move->capture != 0){
				t->piezas[!c][move->capture] &= ~(C64(1) << move->to);
			}
			t->piezas[c][move->piece] &= ~(C64(1) << move->to);
			t->piezas[c][move->promoPiece] |= (C64(1) << move->to);
			break;
	}
	updateBoardCache(t);
}
bool inputAvaliable(){
	struct timeval tv = {0, 0};
	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(0, &fds);
	int ret = select(1, &fds, NULL, NULL, &tv);
	return (ret >  0);
}
void proccesUCICommands(char command[256], Tablero * t){
	if(strcmp(command, "quit\n") == 0){
		exit(0);
		return;
	}
	else if(strcmp(command, "stop\n") == 0){
		stopRequested = true;
		return;
	}
	else if(strcmp(command, "uci\n") == 0){
		printf("id name Supplycat\n");
		printf("id author  arkar\n");
		printf("uciok\n");
		return;
	}
	else if(strcmp(command, "isready\n") == 0){
		printf("readyok\n");
		return;
	}
	char * firstCommand = strtok(command, "\t\n\r\f\v");
	if(strcmp(firstCommand, "position") == 0){
		char * secondCommand = strtok(NULL, "\t\n\r\f\v");
		if(strcmp(secondCommand, "startpos") == 0){
			initBoard(t);
		}
		else if(strcmp(secondCommand, "fen") == 0){
			char * fenString = strtok(NULL, " ");
			int rank = 7;
			int file = 0;
			for(int i = 0; fenString[i] != '\0'; i++){
				char currentChar = fenString[i];
				if(currentChar == '/'){
					rank--;
					file = 0;
					continue;
				}
				if(isdigit(currentChar)){
					file += currentChar - '0';
					continue;
				}
				else{
					color c = isupper(currentChar) ? blancas : negras;
					tipoDePieza piece = charToPiece(currentChar);
					casilla square = rank * 8 + file;
					t->piezas[c][piece] |= BB_SQUARE(square);
					file++;
				}
			}
			updateBoardCache(t);
			char * activeColor = strtok(NULL, " ");
			colorToMove = strcmp(activeColor, "w") ? negras : blancas;
			char * castlingRights = strtok(NULL, " ");
			if(strchr(castlingRights, '-') != NULL){
				t->castlingRights = 0;
			}
			else if(strchr(castlingRights, 'K') != NULL){
				t->castlingRights |= WHITE_OO;
			}
			else if(strchr(castlingRights, 'Q') != NULL){
				t->castlingRights |= WHITE_OOO;
			}
			else if(strchr(castlingRights, 'k') != NULL){
				t->castlingRights |= BLACK_OO;
			}
			else if(strchr(castlingRights, 'q') != NULL){
				t->castlingRights |= BLACK_OOO;
			}
			char * enPassantSquare = strtok(NULL, " ");
			if(strchr(enPassantSquare, '-')){
				t->enPassantSquare = -1;
			}
			else{
				casilla sq = stringToSq(enPassantSquare);
				t->enPassantSquare  = sq;
			}
			char * halfmoveClock = strtok(NULL, " ");
			t->halfmoveClock = atoi(halfmoveClock);
			char * fullMove = strtok(NULL, " ");
			t->fullMoves = atoi(fullMove);
			/*
			TODO: moves tokens
			*/
		}
	}
}
tipoDePieza charToPiece(char c){
	switch (c){
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
	printf("error: invalid fen\n");
}
casilla stringToSq(const char * sq){
	int file = tolower(sq[0]) - 'a';
	int rank = sq[1] - '1';
	return rank * 8 + file;
}
