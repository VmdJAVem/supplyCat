#include <sys/types.h>
#define _POSIX_C_SOURCE 199309L
#include <ctype.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <time.h>
#include <unistd.h>
typedef uint64_t bitboard;
// macros
#define C64(constantU64) constantU64##ULL
#define BB_SQUARE(sq) (1ULL << (sq))
#define RANK(r) (C64(0xFF) << (8 * (r)))
#define C8 ((uint8_t)123)
#define MAX_DEPTH 64
// enums
typedef enum { blancas, negras } color;
typedef enum { peon, caballo, alfil, torre, reina, rey } tipoDePieza;
int sortingValues[6] = {100, 300, 350, 500, 900};
// clangd_format off
typedef enum {
	// Rank 1
	a1,
	b1,
	c1,
	d1,
	e1,
	f1,
	g1,
	h1,
	// Rank 2
	a2,
	b2,
	c2,
	d2,
	e2,
	f2,
	g2,
	h2,
	// Rank 3
	a3,
	b3,
	c3,
	d3,
	e3,
	f3,
	g3,
	h3,
	// Rank 4
	a4,
	b4,
	c4,
	d4,
	e4,
	f4,
	g4,
	h4,
	// Rank 5
	a5,
	b5,
	c5,
	d5,
	e5,
	f5,
	g5,
	h5,
	// Rank 6
	a6,
	b6,
	c6,
	d6,
	e6,
	f6,
	g6,
	h6,
	// Rank 7
	a7,
	b7,
	c7,
	d7,
	e7,
	f7,
	g7,
	h7,
	// Rank 8
	a8,
	b8,
	c8,
	d8,
	e8,
	f8,
	g8,
	h8
} casilla;
typedef struct {
	bitboard piezas[2][6];
	bitboard allPieces[2];
	bitboard allOccupiedSquares;
	uint8_t castlingRights;
	casilla enPassantSquare;
	int halfmoveClock;
	int fullMoves;
	uint64_t hash;
} Tablero;
const uint8_t WHITE_OO = 1;
const uint8_t WHITE_OOO = 2;
const uint8_t BLACK_OO = 4;
const uint8_t BLACK_OOO = 8;

typedef struct {
	casilla from;
	casilla to;
	tipoDePieza piece;
	int capture;
	int special;	// 0 normal, 1 en passant, 2 castling, 3 promo,
	int promoPiece; // 0 none
} Move;
typedef struct {
	Move moves[256];
	int count;
} moveLists;
typedef struct {
	Move move;
	float score;
} moveScore;
typedef struct {
	int wtime;
	int btime;
	int winc;
	int binc;
	int movestogo;
	int depth;
	int movetime;
	bool infinite;
} goParameters;
typedef struct {
	int sortingScore;
	Move move;
} moveSort;
typedef struct {
	uint64_t pieces[2][6][64];
	uint64_t side;
	uint64_t castling[4];
	uint64_t enPassant[8];
} Zobrist;
// atack mascs
bitboard knightAttacks[64];
bitboard kingAttacks[64];
bitboard pawnAttacks[2][64];
// sliding pieces offsets
bitboard rookMask[64][4];
bitboard bishopMask[64][4];
// misc
Tablero tablero = {0};
volatile bool stopRequested = false;
bool isPlaying = true;
color colorToMove;
bool debug = false;
Move killerMoves[MAX_DEPTH][2] = {0};
int history[64][64] = {0};
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
int positionalValues[2][6][64] = {
    // blancas
    {// peon
     {0,  0,  0,  0,  0,  0,  0,  0,  50, 50, 50, 50, 50, 50, 50, 50, 30, 30, 30, 40, 40, 30,
      30, 30, 20, 20, 20, 30, 30, 20, 20, 20, 10, 10, 10, 20, 20, 10, 10, 10, 5,  5,  10, 10,
      10, 10, 5,  5,  0,  0,  0,  5,  5,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
     // caballo
     {-50, -40, -30, -30, -30, -30, -40, -50, -40, -20, 0,   0,	  0,   0,   -20, -40, -30, 0,	10,  15, 15, 10,
      0,   -30, -30, 5,	  15,  20,  20,	 15,  5,   -30, -30, 0,	  15,  20,  20,	 15,  0,   -30, -30, 5,	 10, 15,
      15,  10,	5,   -30, -40, -20, 0,	 5,   5,   0,	-20, -40, -50, -40, -30, -30, -30, -30, -40, -50},
     // alfil
     {-20, -10, -10, -10, -10, -10, -10, -20, -10, 0,	0,   0,	  0,   0,   0,	 -10, -10, 0,	5,   10, 10, 5,
      0,   -10, -10, 5,	  5,   10,  10,	 5,   5,   -10, -10, 0,	  10,  10,  10,	 10,  0,   -10, -10, 10, 10, 10,
      10,  10,	10,  -10, -10, 5,   0,	 0,   0,   0,	5,   -10, -20, -10, -10, -10, -10, -10, -10, -20},
     // torre
     {0,  0, 0, 0, 0, 0, 0, 0,	5,  10, 10, 10, 10, 10, 10, 5,	-5, 0, 0, 0, 0, 0, 0, -5, -5, 0, 0, 0, 0, 0, 0, -5,
      -5, 0, 0, 0, 0, 0, 0, -5, -5, 0,	0,  0,	0,  0,	0,  -5, -5, 0, 0, 0, 0, 0, 0, -5, 0,  0, 0, 5, 5, 0, 0, 0},
     // reina
     {-20, -10, -10, -5,  -5,  -10, -10, -20, -10, 0,  0, 0,   0,   0,	 0,   -10, -10, 0,   5,	  5,  5, 5,
      0,   -10, -5,  0,	  5,   5,   5,	 5,   0,   -5, 0, 0,   5,   5,	 5,   5,   0,	-5,  -10, 5,  5, 5,
      5,   5,	0,   -10, -10, 0,   5,	 0,   0,   0,  0, -10, -20, -10, -10, -5,  -5,	-10, -10, -20},
     // rey
     {-30, -40, -40, -50, -50, -40, -40, -30, -30, -40, -40, -50, -50, -40, -40, -30, -30, -40, -40, -50, -50, -40,
      -40, -30, -30, -40, -40, -50, -50, -40, -40, -30, -20, -30, -30, -40, -40, -30, -30, -20, -10, -20, -20, -20,
      -20, -20, -20, -10, 20,  20,  0,	 0,   0,   0,	20,  20,  20,  30,  10,	 0,   0,   10,	30,  20}},
    // negras
    {// peon
     {0,  0,  0,  0,  0,  0,  0,  0,  50, 50, 50, 50, 50, 50, 50, 50, 30, 30, 30, 40, 40, 30,
      30, 30, 20, 20, 20, 30, 30, 20, 20, 20, 10, 10, 10, 20, 20, 10, 10, 10, 5,  5,  10, 10,
      10, 10, 5,  5,  0,  0,  0,  5,  5,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
     // caballo
     {-50, -40, -30, -30, -30, -30, -40, -50, -40, -20, 0,   0,	  0,   0,   -20, -40, -30, 0,	10,  15, 15, 10,
      0,   -30, -30, 5,	  15,  20,  20,	 15,  5,   -30, -30, 0,	  15,  20,  20,	 15,  0,   -30, -30, 5,	 10, 15,
      15,  10,	5,   -30, -40, -20, 0,	 5,   5,   0,	-20, -40, -50, -40, -30, -30, -30, -30, -40, -50},
     // alfil
     {-20, -10, -10, -10, -10, -10, -10, -20, -10, 0,	0,   0,	  0,   0,   0,	 -10, -10, 0,	5,   10, 10, 5,
      0,   -10, -10, 5,	  5,   10,  10,	 5,   5,   -10, -10, 0,	  10,  10,  10,	 10,  0,   -10, -10, 10, 10, 10,
      10,  10,	10,  -10, -10, 5,   0,	 0,   0,   0,	5,   -10, -20, -10, -10, -10, -10, -10, -10, -20},
     // torre
     {0,  0, 0, 5, 5, 0, 0, 0,	-5, 0, 0, 0, 0, 0, 0, -5, -5, 0,  0,  0,  0,  0,  0,  -5, -5, 0, 0, 0, 0, 0, 0, -5,
      -5, 0, 0, 0, 0, 0, 0, -5, -5, 0, 0, 0, 0, 0, 0, -5, 5,  10, 10, 10, 10, 10, 10, 5,  0,  0, 0, 0, 0, 0, 0, 0},
     // reina
     {-20, -10, -10, -5,  -5,  -10, -10, -20, -10, 0,  5,  0,	0,   0,	  0,   -10, -10, 5,   5,   5,  5, 5,
      0,   -10, 0,   0,	  5,   5,   5,	 5,   0,   -5, -5, 0,	5,   5,	  5,   5,   0,	 -5,  -10, 0,  5, 5,
      5,   5,	0,   -10, -10, 0,   0,	 0,   0,   0,  0,  -10, -20, -10, -10, -5,  -5,	 -10, -10, -20},
     // rey
     {20,  30,	10,  0,	  0,   10,  30,	 20,  20,  20,	0,   0,	  0,   0,   20,	 20,  -10, -20, -20, -20, -20, -20,
      -20, -10, -20, -30, -30, -40, -40, -30, -30, -20, -30, -40, -40, -50, -50, -40, -40, -30, -30, -40, -40, -50,
      -50, -40, -40, -30, -30, -40, -40, -50, -50, -40, -40, -30, -30, -40, -40, -50, -50, -40, -40, -30}}};

// funciones
void printBitboard(bitboard bb);
void initBoard(Tablero * t);
void updateBoardCache(Tablero * t);
void generateAllMoves(color c, Tablero * t, moveLists * output);
void generateCaptures(color c, Tablero * t, moveLists * output);
static inline bool isAttacked(Tablero * t, int square, color attackerColor);
bitboard computeKnightAttacks(casilla sq);
bitboard computeKingAttacks(casilla sq);
bitboard computePawnAttacks(color c, casilla sq);
void initAttackTables();
void generateKnightMoves(moveLists * ml, color c, Tablero * t);
void generateKingMoves(moveLists * ml, color c, Tablero * t);
void generatePawnMoves(moveLists * ml, color c, Tablero * t);
void generateRookMoves(moveLists * ml, color c, Tablero * t);
void generateBishopMoves(moveLists * ml, color c, Tablero * t);
void generateQueenMoves(moveLists * ml, color c, Tablero * t);
float boardEval(Tablero * t, color c);
void makeMove(Move * move, Tablero * t, color c);
moveScore negaMax(Tablero * t, color c, int timeLimit);
float recursiveNegaMax(int depth, Tablero * t, color c, float alpha, float beta);
void proccesUCICommands(char command[256], Tablero * t);
bool inputAvaliable();
static inline tipoDePieza charToPiece(char c);
static inline casilla stringToSq(const char * sq);
static inline char pieceToChar(tipoDePieza piece);
static inline char * moveToStr(Move * move);
moveScore negaMaxFixedDepth(Tablero * t, color c, int depth);
static inline bool isEqualMoves(Move * x, Move * y);
static inline int partition(moveSort arr[], int low, int high);
static inline void swapMoveSort(moveSort * a, moveSort * b);
static inline void moveToMoveSort(moveLists * input, moveSort output[], int depth);
moveSort scoreMoveForSorting(Move * move, int depth);
static inline int compareMoveSort(const void * a, const void * b);
float quiescence(Tablero * t, color c, float alpha, float beta);
bitboard attackedByColor(Tablero * t, color attacker);
void initZobrist();
uint64_t computeZobrist(Zobrist * z, Tablero * t, color sideToMove);
int main() {
	initAttackTables();
	initZobrist();
	/*
	Tablero t;
	initBoard(&t);
	struct timespec start, end;
	clock_gettime(CLOCK_MONOTONIC, &start);
	moveScore best = negaMaxFixedDepth(&t, blancas, 10);
	clock_gettime(CLOCK_MONOTONIC, &end);
	double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
	printf("info nodes %lld nps %.0f\n", nodes, nodes / elapsed);
	*/
	while (true) {
		if (inputAvaliable()) {
			char buffer[256];
			if (fgets(buffer, sizeof(buffer), stdin)) {
				proccesUCICommands(buffer, &tablero);
			}
		}
	}
}
// debug
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
float recursiveNegaMax(int depth, Tablero * t, color c, float alpha, float beta) {
	nodes++;
	if (debug) {
		printf("DEBUG: recursiveNegaMax start, colorToMove = %d\n", c);
	}
	moveLists colorToMove = {0};
	generateAllMoves(c, t, &colorToMove);
	if (depth == 0 || colorToMove.count == 0) {
		return quiescence(t, c, alpha, beta);
	}
	if (nodes % 131072 == 0) {
		if (inputAvaliable()) {
			char buffer[256];
			if (fgets(buffer, sizeof(buffer), stdin)) {
				proccesUCICommands(buffer, t);
			}
		}
	}
	moveSort moves[256];
	moveToMoveSort(&colorToMove, moves, depth);
	qsort(moves, colorToMove.count, sizeof(moveSort), compareMoveSort);
	for (int i = 0; i < colorToMove.count; i++) {
		Tablero temp = *t;
		Move move = moves[i].move;
		makeMove(&move, &temp, c);
		float score = -recursiveNegaMax(depth - 1, &temp, !c, -beta, -alpha);
		if (score > alpha) {
			alpha = score;
		}
		if (alpha >= beta) {
			if (moves[i].move.capture == 0 && moves[i].move.capture != 3) {
				history[moves[i].move.from][moves[i].move.to] += depth * depth;
				if (isEqualMoves(&move, &killerMoves[depth][0])) {
					continue;
				} else {
					killerMoves[depth][1] = killerMoves[depth][0];
					killerMoves[depth][0] = move;
				}
			}
			break;
		}
	}
	return alpha;
}
moveScore negaMax(Tablero * t, color c, int timeLimit) {
	if (debug) {
		printf("DEBUG: negaMax start, colorToMove = %d\n", colorToMove);
	}
	memset(history, 0, sizeof(history));
	fflush(stdout);
	moveLists colorToMove = {0};
	generateAllMoves(c, t, &colorToMove);
	if (debug) {
		printf("DEBUG: generateAllMoves returned %d moves\n", colorToMove.count);
		fflush(stdout);
	}
	float bestScore = -INFINITY;
	Move bestMove = {0};
	float alpha = -INFINITY;
	float beta = +INFINITY;
	stopRequested = false;
	struct timespec start;
	clock_gettime(CLOCK_MONOTONIC, &start);
	if (colorToMove.count == 0) {
		moveScore output = {
		    .move = {0},
		    .score = boardEval(t, c),
		};
		if (debug) {
			printf("DEBUG: board state\n");
			printBitboard(t->allOccupiedSquares);
		}
		return output;
	}
	int depth = 1;
	while (!stopRequested) {
		Move localBestMove = bestMove;
		float localBestScore = bestScore;
		for (int i = 0; i < colorToMove.count; i++) {
			if (nodes % 131072 == 0) {
				if (inputAvaliable()) {
					char buffer[256];
					if (fgets(buffer, sizeof(buffer), stdin)) {
						proccesUCICommands(buffer, t);
					}
				}
			}
			Tablero temp = *t;
			makeMove(&colorToMove.moves[i], &temp, c);
			float score = -recursiveNegaMax(depth, &temp, !c, -beta, -alpha);
			if (score > localBestScore) {
				localBestScore = score;
				alpha = score;
				localBestMove = colorToMove.moves[i];
			}
			if (timeLimit != -1) {
				struct timespec now;
				clock_gettime(CLOCK_MONOTONIC, &now);
				long long elapsed =
				    (now.tv_sec - start.tv_sec) * 1000LL + (now.tv_nsec - start.tv_nsec) / 1000000LL;
				if (elapsed >= timeLimit) {
					stopRequested = true;
					break;
				}
			}
		}
		depth++;
		if (localBestScore > bestScore) {
			bestScore = localBestScore;
			alpha = localBestScore;
			bestMove = localBestMove;
		}
	}
	moveScore bm = {.move = bestMove, .score = bestScore};
	if (debug) {
		printf("%lld nodes searched\n", nodes);
	}
	return bm;
}
moveScore negaMaxFixedDepth(Tablero * t, color c, int depth) {
	moveLists colorToMove = {0};
	generateAllMoves(c, t, &colorToMove);
	memset(history, 0, sizeof(history));

	if (colorToMove.count == 0) {
		moveScore output = {.move = {0}, .score = boardEval(t, c)};
		return output;
	}

	float bestScore = -INFINITY;
	Move bestMove = {0};
	float alpha = -INFINITY;
	float beta = INFINITY;

	for (int i = 0; i < colorToMove.count; i++) {
		if (nodes % 131072 == 0) {
			if (inputAvaliable()) {
				char buffer[256];
				if (fgets(buffer, sizeof(buffer), stdin)) {
					proccesUCICommands(buffer, t);
				}
			}
		}
		Tablero temp = *t;
		makeMove(&colorToMove.moves[i], &temp, c);
		float score = -recursiveNegaMax(depth - 1, &temp, !c, -beta, -alpha);

		if (score > bestScore) {
			bestScore = score;
			bestMove = colorToMove.moves[i];
			alpha = score; // tighten alpha for root pruning
		}
	}

	moveScore result = {.move = bestMove, .score = bestScore};
	if (debug) {
		printf("%lld nodes searched\n", nodes);
	}
	return result;
}
void initBoard(Tablero * t) {
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
	t->hash = computeZobrist(&zobrist, t, blancas);
	updateBoardCache(t);
}
void updateBoardCache(Tablero * t) {
	t->allPieces[blancas] = t->piezas[blancas][peon] | t->piezas[blancas][caballo] | t->piezas[blancas][alfil] |
				t->piezas[blancas][torre] | t->piezas[blancas][reina] | t->piezas[blancas][rey];

	t->allPieces[negras] = t->piezas[negras][peon] | t->piezas[negras][caballo] | t->piezas[negras][alfil] |
			       t->piezas[negras][torre] | t->piezas[negras][reina] | t->piezas[negras][rey];
	t->allOccupiedSquares = t->allPieces[blancas] | t->allPieces[negras];
}
void generateAllMoves(color c, Tablero * t, moveLists * output) {
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
void generateCaptures(color c, Tablero * t, moveLists * output) {
	output->count = 0;
	moveLists temp = {0};
	generateKingMoves(&temp, c, t);
	generateKnightMoves(&temp, c, t);
	generatePawnMoves(&temp, c, t);
	generateRookMoves(&temp, c, t);
	generateBishopMoves(&temp, c, t);
	generateQueenMoves(&temp, c, t);
	for (int i = 0; i < temp.count; i++) {
		if (temp.moves[i].capture != 0) {
			Tablero tempT = *t;
			makeMove(&temp.moves[i], &tempT, c);
			casilla king = __builtin_ctzll(tempT.piezas[c][rey]);
			if (!isAttacked(&tempT, king, !c)) {
				output->moves[output->count] = temp.moves[i];
				output->count++;
			}
		}
	}
}
static inline bool isAttacked(Tablero * t, int square, color attackerColor) {
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
			if (d < 2) // north or east (positive offset)
				firstBlocker = __builtin_ctzll(blockers);
			else // south or west (negative offset)
				firstBlocker = 63 - __builtin_clzll(blockers);

			// Check if the first blocker is an enemy rook or queen
			if ((BB_SQUARE(firstBlocker) &
			     (t->piezas[attackerColor][torre] | t->piezas[attackerColor][reina])))
				return true;
		}
	}

	// Bishops and queens (diagonal)
	for (int d = 0; d < 4; d++) {
		bitboard ray = bishopMask[square][d];
		bitboard blockers = ray & t->allOccupiedSquares;
		if (blockers) {
			int firstBlocker;
			if (d > 1) // down‑left or down‑right (positive offset)
				firstBlocker = __builtin_ctzll(blockers);
			else // up‑left or up‑right (negative offset)
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
	int color = (c == blancas) ? 0 : 1;

	for (int i = 0; i < 2; i++) {
		int newRank = rank + moves[color][i][0];
		int newFile = file + moves[color][i][1];

		if (newRank >= 0 && newRank < 8 && newFile >= 0 && newFile < 8) {
			int destSquare = newRank * 8 + newFile;
			result |= BB_SQUARE(destSquare);
		}
	}

	return result;
}
void initAttackTables() {
	// caballo
	for (int i = 0; i < 64; i++) {
		knightAttacks[i] = computeKnightAttacks(i);
	}
	// rey
	for (int i = 0; i < 64; i++) {
		kingAttacks[i] = computeKingAttacks(i);
	}
	// peon
	for (int i = 0; i < 64; i++) {
		pawnAttacks[blancas][i] = computePawnAttacks(blancas, i);
		pawnAttacks[negras][i] = computePawnAttacks(negras, i);
	}
	// rook masks
	for (int i = 0; i < 64; i++) {
		// north
		bitboard northRay = 0;
		int to = i + 8;
		while (to < 64) {
			northRay |= BB_SQUARE(to);
			to += 8;
		}
		rookMask[i][0] = northRay;
		// east
		bitboard eastRay = 0;
		to = i + 1;
		while (to < 64 && (to / 8) == (i / 8)) {
			eastRay |= BB_SQUARE(to);
			to++;
		}
		rookMask[i][1] = eastRay;
		// west
		bitboard westRay = 0;
		to = i - 1;
		while (to >= 0 && (to / 8) == (i / 8)) {
			westRay |= BB_SQUARE(to);
			to--;
		}
		rookMask[i][2] = westRay;
		// south
		bitboard southRay = 0;
		to = i - 8;
		while (to >= 0) {
			southRay |= BB_SQUARE(to);
			to -= 8;
		}
		rookMask[i][3] = southRay;
	}

	// int bishopOffsets[4] = {-9,-7,7,9}; //up-left,
	// up-right,down-left,down-right
	for (int i = 0; i < 64; i++) {
		// up-left
		bitboard upLeftRay = 0;
		int to = i - 9;
		while (to >= 0 && to < 64 && abs((to % 8) - (i % 8)) == abs((to / 8) - (i / 8))) {
			upLeftRay |= BB_SQUARE(to);
			to -= 9;
		}
		bishopMask[i][0] = upLeftRay;
		// up-right
		bitboard upRightRay = 0;
		to = i - 7;
		while (to >= 0 && to < 64 && abs((to % 8) - (i % 8)) == abs((to / 8) - (i / 8))) {
			upRightRay |= BB_SQUARE(to);
			to -= 7;
		}
		bishopMask[i][1] = upRightRay;
		// down-left
		bitboard downLeftRay = 0;
		to = i + 7;
		while (to >= 0 && to < 64 && abs((to % 8) - (i % 8)) == abs((to / 8) - (i / 8))) {
			downLeftRay |= BB_SQUARE(to);
			to += 7;
		}
		bishopMask[i][2] = downLeftRay;
		// down right
		bitboard downRightRay = 0;
		to = i + 9;
		while (to >= 0 && to < 64 && abs((to % 8) - (i % 8)) == abs((to / 8) - (i / 8))) {
			downRightRay |= BB_SQUARE(to);
			to += 9;
		}
		bishopMask[i][3] = downRightRay;
	}
	for (int i = 0; i < 64; i++) {
	}
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
	if (king == 0) {
		// this should NOT happen
		return;
	}
	if (king & (king - 1)) {
		return;
	}
	casilla from = __builtin_ctzll(king);
	bitboard attacks = kingAttacks[from] & (~t->allPieces[c]);
	for (int i = 0; i < 64; i++) {
		if (attacks & (C64(1) << i)) {
			int capture = -1;
			if (BB_SQUARE(i) & t->allPieces[!c]) {
				for (int piece = peon; piece <= rey; piece++) {
					if (t->piezas[!c][piece] & BB_SQUARE(i)) {
						capture = piece;
						break;
					}
				}
				Move move = {from, i, rey, capture, 0, 0};
				ml->moves[ml->count] = move;
				ml->count++;
			}
		}
	}

	bitboard attacked = attackedByColor(t, !c);
	// castling ROOKS IS HANDLED WHEN MAKING MOVE
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
				      BB_SQUARE(g1) & attacked)) { // is the square we are  moving to attacked || is the
								   // king in check
					casilla to = g1;
					Move move = {from, to, rey, 0, 2, 0};
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
					Move move = {from, to, rey, 0, 2, 0};
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
					Move move = {from, to, rey, 0, 2, 0};
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
					Move move = {from, to, rey, 0, 2, 0};
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
				if ((to / 8 == 1 && c == negras) || (to / 8 == 6 && c == blancas)) {
					for (int i = caballo; i < rey; i++) {
						Move move = {from, to, peon, 3, i};
						ml->moves[ml->count] = move;
						ml->count++;
					}
				} else {
					Move move = {from, to, peon, 0, 0, 0};
					ml->moves[ml->count] = move;
					ml->count++;
				}
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
		// White en passant
		if (rank == 5 && t->enPassantSquare != -1 && c == blancas) {
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
		// Black en passant
		else if (rank == 4 && t->enPassantSquare != -1 && c == negras) {
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
	bitboard allRooks = (t->piezas[c][torre]);
	while (allRooks) {
		casilla from = __builtin_ctzll(allRooks);
		allRooks &= (allRooks - 1);
		for (int i = 0; i < 4; i++) {
			bitboard ray = rookMask[from][i];
			bitboard blockers = ray & t->allOccupiedSquares;
			if (i < 2) {
				if (blockers) {
					int blockerSq = __builtin_ctzll(blockers);
					bitboard before = ray & ((C64(1) << blockerSq) - 1);
					while (before) {
						int to = __builtin_ctzll(before);
						before &= before - 1;
						Move move = {from, to, torre, 0, 0, 0};
						ml->moves[ml->count] = move;
						ml->count++;
					}
					if (BB_SQUARE(blockerSq) & t->allPieces[!c]) {
						int capture = -1;
						for (int p = peon; p < rey; p++) {
							if (t->piezas[!c][p] & BB_SQUARE(blockerSq)) {
								capture = p;
								break;
							}
						}
						Move move = {from, blockerSq, torre, capture, 0, 0};
						ml->moves[ml->count] = move;
						ml->count++;
					}
				} else {
					bitboard targets = ray;
					while (targets) {
						int to = __builtin_ctzll(targets);
						targets &= targets - 1;
						Move move = {from, to, torre, 0, 0, 0};
						ml->moves[ml->count++] = move;
					}
				}
			} else {
				if (blockers) {
					int blockerSq = 63 - __builtin_clzll(blockers);
					bitboard before = ray & ~((1ULL << (blockerSq + 1)) - 1);
					while (before) {
						int to = 63 - __builtin_clzll(before);
						before &= ~(1ULL << to);
						Move move = {from, to, torre, 0, 0, 0};
						ml->moves[ml->count] = move;
						ml->count++;
					}
					if (BB_SQUARE(blockerSq) & t->allPieces[!c]) {
						int capture = -1;
						for (int p = 0; p < rey; p++) {
							if (t->piezas[!c][p] & BB_SQUARE(blockerSq)) {
								capture = p;
								break;
							}
						}
						Move move = {from, blockerSq, torre, capture, 0, 0};
						ml->moves[ml->count] = move;
						ml->count++;
					}
				} else {
					bitboard targets = ray;
					while (targets) {
						int to = 63 - __builtin_clzll(targets);
						targets &= ~(1ULL << to);
						Move move = {from, to, torre, 0, 0, 0};
						ml->moves[ml->count] = move;
						ml->count++;
					}
				}
			}
		}
	}
}
void generateBishopMoves(moveLists * ml, color c, Tablero * t) {
	bitboard allBishops = t->piezas[c][alfil];
	while (allBishops) {
		// int bishopOffsets[4] = {-9,-7,7,9}; //up-left,
		casilla from = __builtin_ctzll(allBishops);
		allBishops &= (allBishops - 1);
		for (int i = 0; i < 4; i++) {
			bitboard ray = bishopMask[from][i];
			bitboard blockers = ray & t->allOccupiedSquares;
			if (i > 1) {
				if (blockers) {
					int blockerSq = __builtin_ctzll(blockers);
					bitboard before = ray & ((C64(1) << blockerSq) - 1);
					while (before) {
						int to = __builtin_ctzll(before);
						before &= before - 1;
						Move move = {from, to, alfil, 0, 0, 0};
						ml->moves[ml->count] = move;
						ml->count++;
					}
					if (BB_SQUARE(blockerSq) & t->allPieces[!c]) {
						int capture = -1;
						for (int p = peon; p < rey; p++) {
							if (t->piezas[!c][p] & BB_SQUARE(blockerSq)) {
								capture = p;
								break;
							}
						}
						Move move = {from, blockerSq, alfil, capture, 0, 0};
						ml->moves[ml->count] = move;
						ml->count++;
					}
				} else {
					bitboard targets = ray;
					while (targets) {
						int to = __builtin_ctzll(targets);
						targets &= targets - 1;
						Move move = {from, to, alfil, 0, 0, 0};
						ml->moves[ml->count++] = move;
					}
				}
			} else {
				if (blockers) {
					int blockerSq = 63 - __builtin_clzll(blockers);
					bitboard before = ray & ~((1ULL << (blockerSq + 1)) - 1);
					while (before) {
						int to = 63 - __builtin_clzll(before);
						before &= ~(1ULL << to);
						Move move = {from, to, alfil, 0, 0, 0};
						ml->moves[ml->count] = move;
						ml->count++;
					}
					if (BB_SQUARE(blockerSq) & t->allPieces[!c]) {
						int capture = -1;
						for (int p = 0; p < rey; p++) {
							if (t->piezas[!c][p] & BB_SQUARE(blockerSq)) {
								capture = p;
								break;
							}
						}
						Move move = {from, blockerSq, alfil, capture, 0, 0};
						ml->moves[ml->count] = move;
						ml->count++;
					}
				} else {
					bitboard targets = ray;
					while (targets) {
						int to = 63 - __builtin_clzll(targets);
						targets &= ~(1ULL << to);
						Move move = {from, to, alfil, 0, 0, 0};
						ml->moves[ml->count] = move;
						ml->count++;
					}
				}
			}
		}
	}
}
void generateQueenMoves(moveLists * ml, color c, Tablero * t) {
	bitboard allQueens = t->piezas[c][reina];
	while (allQueens) {
		int from = __builtin_ctzll(allQueens);
		allQueens &= allQueens - 1;
		for (int i = 0; i < 4; i++) {
			bitboard ray = bishopMask[from][i];
			bitboard blockers = ray & t->allOccupiedSquares;
			if (i > 1) {
				if (blockers) {
					int blockerSq = __builtin_ctzll(blockers);
					bitboard before = ray & ((C64(1) << blockerSq) - 1);
					while (before) {
						int to = __builtin_ctzll(before);
						before &= before - 1;
						Move move = {from, to, reina, 0, 0, 0};
						ml->moves[ml->count] = move;
						ml->count++;
					}
					if (BB_SQUARE(blockerSq) & t->allPieces[!c]) {
						int capture = -1;
						for (int p = peon; p < rey; p++) {
							if (t->piezas[!c][p] & BB_SQUARE(blockerSq)) {
								capture = p;
								break;
							}
						}
						Move move = {from, blockerSq, reina, capture, 0, 0};
						ml->moves[ml->count] = move;
						ml->count++;
					}
				} else {
					bitboard targets = ray;
					while (targets) {
						int to = __builtin_ctzll(targets);
						targets &= targets - 1;
						Move move = {from, to, reina, 0, 0, 0};
						ml->moves[ml->count++] = move;
					}
				}
			} else {
				if (blockers) {
					int blockerSq = 63 - __builtin_clzll(blockers);
					bitboard before = ray & ~((1ULL << (blockerSq + 1)) - 1);
					while (before) {
						int to = 63 - __builtin_clzll(before);
						before &= ~(1ULL << to);
						Move move = {from, to, reina, 0, 0, 0};
						ml->moves[ml->count] = move;
						ml->count++;
					}
					if (BB_SQUARE(blockerSq) & t->allPieces[!c]) {
						int capture = -1;
						for (int p = 0; p < rey; p++) {
							if (t->piezas[!c][p] & BB_SQUARE(blockerSq)) {
								capture = p;
								break;
							}
						}
						Move move = {from, blockerSq, reina, capture, 0, 0};
						ml->moves[ml->count] = move;
						ml->count++;
					}
				} else {
					bitboard targets = ray;
					while (targets) {
						int to = 63 - __builtin_clzll(targets);
						targets &= ~(1ULL << to);
						Move move = {from, to, reina, 0, 0, 0};
						ml->moves[ml->count] = move;
						ml->count++;
					}
				}
			}
		}
		for (int i = 0; i < 4; i++) {
			bitboard ray = rookMask[from][i];
			bitboard blockers = ray & t->allOccupiedSquares;
			if (i < 2) {
				if (blockers) {
					int blockerSq = __builtin_ctzll(blockers);
					bitboard before = ray & ((C64(1) << blockerSq) - 1);
					while (before) {
						int to = __builtin_ctzll(before);
						before &= before - 1;
						Move move = {from, to, reina, 0, 0, 0};
						ml->moves[ml->count] = move;
						ml->count++;
					}
					if (BB_SQUARE(blockerSq) & t->allPieces[!c]) {
						int capture = -1;
						for (int p = peon; p < rey; p++) {
							if (t->piezas[!c][p] & BB_SQUARE(blockerSq)) {
								capture = p;
								break;
							}
						}
						Move move = {from, blockerSq, reina, capture, 0, 0};
						ml->moves[ml->count] = move;
						ml->count++;
					}
				} else {
					bitboard targets = ray;
					while (targets) {
						int to = __builtin_ctzll(targets);
						targets &= targets - 1;
						Move move = {from, to, reina, 0, 0, 0};
						ml->moves[ml->count++] = move;
					}
				}
			} else {
				if (blockers) {
					int blockerSq = 63 - __builtin_clzll(blockers);
					bitboard before = ray & ~((1ULL << (blockerSq + 1)) - 1);
					while (before) {
						int to = 63 - __builtin_clzll(before);
						before &= ~(1ULL << to);
						Move move = {from, to, reina, 0, 0, 0};
						ml->moves[ml->count] = move;
						ml->count++;
					}
					if (BB_SQUARE(blockerSq) & t->allPieces[!c]) {
						int capture = -1;
						for (int p = 0; p < rey; p++) {
							if (t->piezas[!c][p] & BB_SQUARE(blockerSq)) {
								capture = p;
								break;
							}
						}
						Move move = {from, blockerSq, reina, capture, 0, 0};
						ml->moves[ml->count] = move;
						ml->count++;
					}
				} else {
					bitboard targets = ray;
					while (targets) {
						int to = 63 - __builtin_clzll(targets);
						targets &= ~(1ULL << to);
						Move move = {from, to, reina, 0, 0, 0};
						ml->moves[ml->count] = move;
						ml->count++;
					}
				}
			}
		}
	}
}
float boardEval(Tablero * t, color c) {
	// queen = 9; rook = 5; bishop = 3; knight = 3; pawn = 1;
	if (t->piezas[c][rey] == 0) {
		return 0;
	};
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
	float positional = 0;
	for (tipoDePieza p = 0; p <= rey; p++) {
		bitboard pieces = t->piezas[c][p];
		while (pieces) {
			int sq = __builtin_ctzll(pieces);
			pieces &= pieces - 1;
			positional += positionalValues[c][p][sq];
		}
	}
	for (tipoDePieza p = 0; p <= rey; p++) {
		bitboard pieces = t->piezas[!c][p];
		while (pieces) {
			int sq = __builtin_ctzll(pieces);
			pieces &= pieces - 1;
			positional -= positionalValues[!c][p][sq];
		}
	}

	float value =
	    100 * (__builtin_popcountll(t->piezas[c][peon]) - __builtin_popcountll(t->piezas[!c][peon])) +
	    300 * (__builtin_popcountll(t->piezas[c][caballo]) - __builtin_popcountll(t->piezas[!c][caballo])) +
	    350 * (__builtin_popcountll(t->piezas[c][alfil]) - __builtin_popcountll(t->piezas[!c][alfil])) +
	    500 * (__builtin_popcountll(t->piezas[c][torre]) - __builtin_popcountll(t->piezas[!c][torre])) +
	    900 * (__builtin_popcountll(t->piezas[c][reina]) - __builtin_popcountll(t->piezas[!c][reina])) +
	    10 * (movePerColor[c].count - movePerColor[!c].count);
	value += positional;
	if (debug) {
		printf("%f\n", value);
	}
	return value;
}

void makeMove(Move * move, Tablero * t, color c) {
	casilla enPassantSq = t->enPassantSquare;
	t->enPassantSquare = -1;
	t->fullMoves++;
	if (move->piece == peon) {
		t->halfmoveClock = 0;
	}
	switch (move->special) {
	case 0:
		if (move->capture != -1) {
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
			case h1:
				t->castlingRights &= ~WHITE_OO;
				break;
			case a1:
				t->castlingRights &= ~WHITE_OOO;
				break;
			case h8:
				t->castlingRights &= ~BLACK_OO;
				break;
			case a8:
				t->castlingRights &= ~BLACK_OOO;
				break;
			default:
				break;
			}
		}
		if (move->piece == torre) {
			casilla from = move->from;
			switch (from) {
			case h1:
				t->castlingRights &= ~WHITE_OO;
				break;
			case a1:
				t->castlingRights &= ~WHITE_OOO;
				break;
			case h8:
				t->castlingRights &= ~BLACK_OO;
				break;
			case a8:
				t->castlingRights &= ~BLACK_OOO;
				break;
			default:
				break;
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
		casilla opponentPawn = (c == blancas ? enPassantSq - 8 : enPassantSq + 8);
		t->piezas[c][move->piece] &= ~(C64(1) << move->from);
		t->piezas[c][move->piece] |= (C64(1) << move->to);
		t->piezas[!c][peon] &= ~(C64(1) << opponentPawn);
		t->allPieces[!c] &= ~(BB_SQUARE(opponentPawn));
		t->allOccupiedSquares &= ~(BB_SQUARE(opponentPawn));
		break;
	case 2:
		// castling
		t->piezas[c][move->piece] &= ~(C64(1) << move->from);
		t->piezas[c][move->piece] |= (C64(1) << move->to);
		switch (move->to) {
		case g1:
			if (BB_SQUARE(h1) & t->piezas[c][torre]) {
				t->piezas[c][torre] &= ~BB_SQUARE(h1);
				t->piezas[c][torre] |= BB_SQUARE(f1);
				t->castlingRights &= ~WHITE_OO;
				t->castlingRights &= ~WHITE_OOO;
				// move rook
				t->allPieces[c] &= ~(BB_SQUARE(h1));
				t->allPieces[c] |= BB_SQUARE(f1);
				t->allOccupiedSquares &= ~(BB_SQUARE(h1));
				t->allOccupiedSquares |= BB_SQUARE(f1);
			}
			break;
		case c1:
			if (BB_SQUARE(a1) & t->piezas[c][torre]) {
				t->piezas[c][torre] &= ~BB_SQUARE(a1);
				t->piezas[c][torre] |= BB_SQUARE(d1);
				t->castlingRights &= ~WHITE_OO;
				t->castlingRights &= ~WHITE_OOO;
				t->allOccupiedSquares &= ~(BB_SQUARE(a1));
				t->allOccupiedSquares |= BB_SQUARE(d1);
				t->allPieces[c] &= ~(BB_SQUARE(a1));
				t->allPieces[c] |= BB_SQUARE(d1);
			}
			break;
		case g8:
			if (BB_SQUARE(h8) & t->piezas[c][torre]) {
				t->piezas[c][torre] &= ~BB_SQUARE(h8);
				t->piezas[c][torre] |= BB_SQUARE(f8);
				t->castlingRights &= ~BLACK_OO;
				t->castlingRights &= ~BLACK_OOO;
				t->allPieces[c] &= ~(BB_SQUARE(h8));
				t->allPieces[c] |= BB_SQUARE(f8);

				t->allOccupiedSquares &= ~(BB_SQUARE(h8));
				t->allOccupiedSquares |= BB_SQUARE(f8);
			}
			break;
		case c8:
			if (BB_SQUARE(a8) & t->piezas[c][torre]) {
				t->piezas[c][torre] &= ~BB_SQUARE(a8);
				t->piezas[c][torre] |= BB_SQUARE(f8);

				t->castlingRights &= ~BLACK_OO;
				t->castlingRights &= ~BLACK_OOO;
				t->allPieces[c] &= ~(BB_SQUARE(a8));
				t->allPieces[c] |= BB_SQUARE(d8);
				t->allOccupiedSquares &= ~(BB_SQUARE(a8));
				t->allOccupiedSquares |= BB_SQUARE(d8);
			}
			break;
		default:
			break;
		}
		break;

	case 3:
		// promotion
		t->piezas[c][move->piece] &= ~(C64(1) << move->from);
		if (move->capture != -1) {
			t->piezas[!c][move->capture] &= ~(C64(1) << move->to);
		}
		t->piezas[c][move->piece] &= ~(C64(1) << move->to);
		t->piezas[c][move->promoPiece] |= (C64(1) << move->to);
		break;
	}
	t->allPieces[c] &= ~(BB_SQUARE(move->from));
	t->allPieces[c] |= BB_SQUARE(move->to);
	t->allOccupiedSquares &= ~(BB_SQUARE(move->from));
	t->allOccupiedSquares |= BB_SQUARE(move->to);
	if (move->capture != -1) {
		t->allPieces[!c] &= ~(BB_SQUARE(move->to));
	}
	// zobrist
	t->hash ^= zobrist.pieces[c][move->piece][move->from];
	if (move->capture != -1) {
	}
}
bool inputAvaliable() {
	struct timeval tv = {0, 0};
	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(0, &fds);
	int ret = select(1, &fds, NULL, NULL, &tv);
	return (ret > 0);
}
void proccesUCICommands(char command[256], Tablero * t) {
	fflush(stdout);
	if (strcmp(command, "quit\n") == 0) {
		exit(0);
		return;
	} else if (strcmp(command, "stop\n") == 0) {
		stopRequested = true;
		return;
	} else if (strcmp(command, "uci\n") == 0) {
		printf("id name Supplycat\n");
		printf("id author  arkar\n");
		printf("uciok\n");
		return;
	} else if (strcmp(command, "isready\n") == 0) {
		printf("readyok\n");
		return;
	}
	if (debug) {
		printf("DEBUG: full command = '%s'\n", command);
		fflush(stdout);
	}
	char * firstCommand = strtok(command, " \t\n\r\f\v");
	if (debug) {
		printf("DEBUG: firstCommand = '%s'\n", firstCommand);
		fflush(stdout);
	}

	if (strcmp(firstCommand, "debug") == 0) {
		char * x = strtok(NULL, " \t\n\r\f\v");
		if (strcmp(x, "off") == 0) {
			printf("ENDING DEBUGING\n");
			debug = false;
		} else if (strcmp(x, "on") == 0) {
			debug = true;
			printf("DEBUGING\n");
		}
	}
	if (strcmp(firstCommand, "position") == 0) {
		if (debug) {
			printBitboard(t->allOccupiedSquares);
		}
		char * secondCommand = strtok(NULL, " \t\n\r\f\v");
		if (debug) {
			printf("DEBUG: secondCommand = '%s'\n", secondCommand);
		}
		if (strcmp(secondCommand, "startpos") == 0) {
			initBoard(t);
			if (debug) {
				printf("DEBUG: after initBoard, white pawns = "
				       "%lx\n",
				       t->piezas[blancas][peon]);
			}
			colorToMove = blancas;
			char * movesToken = strtok(NULL, " ");
			if (movesToken != NULL && strcmp(movesToken, "moves") == 0) {
				char * move = strtok(NULL, " ");
				while (move != NULL) {
					casilla to = (stringToSq(move + 2));
					casilla from = (stringToSq(move));
					tipoDePieza promo = 0;
					tipoDePieza piece = -1;
					int special = 0;
					tipoDePieza capture = 0;
					for (int i = peon; i <= rey; i++) {
						if (BB_SQUARE(from) & t->piezas[colorToMove][i]) {
							piece = i;
							break;
						}
					}
					if (piece == -1) {
						printf("ERROR: INVALID MOVE\n");
						exit(256);
					}
					if (move[4] != '\0') {
						promo = charToPiece(move[4]);
						special = 3;
					}
					if (BB_SQUARE(to) & t->allPieces[!colorToMove]) {
						for (int p = peon; p <= rey; p++) {
							if (t->piezas[!colorToMove][p] & BB_SQUARE(to)) {
								capture = p;
								break;
							}
						}
					}
					if (piece == peon && to == t->enPassantSquare) {
						special = 1;
					}
					if (piece == rey) {
						if ((colorToMove == blancas && from == e1 && (to == g1 || to == c1)) ||
						    (colorToMove == negras && from == e8 && (to == g8 || to == c8))) {
							special = 2;
						}
					}
					Move newMove = {from, to, piece, capture, special, promo};
					makeMove(&newMove, t, colorToMove);
					move = strtok(NULL, " ");
					colorToMove = !colorToMove;
				}
			}
			if (debug) {
				printBitboard(t->allOccupiedSquares);
			}
		} else if (strcmp(secondCommand, "fen") == 0) {
			memset(t, 0, sizeof(Tablero));
			char * fenString = strtok(NULL, " \t\n\r\f\v");
			if (debug) {
				printf("DEBUG: fenString = '%s'\n", fenString);
			}
			int rank = 7;
			int file = 0;
			for (int i = 0; fenString[i] != '\0'; i++) {
				char currentChar = fenString[i];
				if (currentChar == '/') {
					rank--;
					file = 0;
					continue;
				}
				if (isdigit(currentChar)) {
					file += currentChar - '0';
					if (file > 7) {
						file = 7;
					}
					continue;
				} else {
					color c = isupper(currentChar) ? blancas : negras;
					tipoDePieza piece = charToPiece(currentChar);
					casilla square = rank * 8 + file;
					t->piezas[c][piece] |= BB_SQUARE(square);
					file++;
					if (debug) {
						printf("Placing %c at square %d\n", currentChar, square);
					}
				}
			}
			updateBoardCache(t);
			char * activeColor = strtok(NULL, " ");
			colorToMove = strcmp(activeColor, "w") ? negras : blancas;
			char * castlingRights = strtok(NULL, " ");
			if (strchr(castlingRights, '-') != NULL) {
				t->castlingRights = 0;
			}
			if (strchr(castlingRights, 'K') != NULL) {
				t->castlingRights |= WHITE_OO;
			}
			if (strchr(castlingRights, 'Q') != NULL) {
				t->castlingRights |= WHITE_OOO;
			}
			if (strchr(castlingRights, 'k') != NULL) {
				t->castlingRights |= BLACK_OO;
			}
			if (strchr(castlingRights, 'q') != NULL) {
				t->castlingRights |= BLACK_OOO;
			}
			char * enPassantSquare = strtok(NULL, " ");
			if (strchr(enPassantSquare, '-')) {
				t->enPassantSquare = -1;
			} else {
				casilla sq = stringToSq(enPassantSquare);
				t->enPassantSquare = sq;
			}
			char * halfmoveClock = strtok(NULL, " ");
			t->halfmoveClock = atoi(halfmoveClock);
			char * fullMove = strtok(NULL, " ");
			t->fullMoves = atoi(fullMove);
			char * movesToken = strtok(NULL, " ");
			if (movesToken != NULL && strcmp(movesToken, "moves") == 0) {
				char * move = strtok(NULL, " ");
				while (move != NULL) {
					casilla to = (stringToSq(move + 2));
					casilla from = (stringToSq(move));
					tipoDePieza promo = 0;
					tipoDePieza piece = -1;
					int special = 0;
					tipoDePieza capture = 0;
					for (int i = peon; i <= rey; i++) {
						if (BB_SQUARE(from) & t->piezas[colorToMove][i]) {
							piece = i;
							break;
						}
					}
					if (piece == -1) {
						printf("ERROR: INVALID MOVE\n");
						exit(256);
					}
					if (move[4] != '\0') {
						promo = charToPiece(move[4]);
						special = 3;
					}
					if (BB_SQUARE(to) & t->allPieces[!colorToMove]) {
						for (int p = peon; p <= rey; p++) {
							if (t->piezas[!colorToMove][p] & BB_SQUARE(to)) {
								capture = p;
								break;
							}
						}
					}
					if (piece == peon && to == t->enPassantSquare) {
						special = 1;
					}
					if (piece == rey) {
						if ((colorToMove == blancas && from == e1 && (to == g1 || to == c1)) ||
						    (colorToMove == negras && from == e8 && (to == g8 || to == c8))) {
							special = 2;
						}
					}
					Move newMove = {from, to, piece, capture, special, promo};
					makeMove(&newMove, t, colorToMove);
					move = strtok(NULL, " ");
					colorToMove = !colorToMove;
				}
			}

			t->hash = computeZobrist(&zobrist, t, colorToMove);
		}
	} else if (strcmp(firstCommand, "go") == 0 || strcmp(firstCommand, "go\n") == 0) {
		parameters = (goParameters){
		    .wtime = -1,
		    .btime = -1,
		    .winc = -1,
		    .binc = -1,
		    .movestogo = -1,
		    .depth = -1,
		    .movetime = -1,
		    .infinite = false,
		};
		char * secondCommand = strtok(NULL, " \t\n\r\f\v");
		char possible2ndCommands[8][10] = {"wtime",	"btime", "winc",     "binc",
						   "movestogo", "depth", "movetime", "infinite"};
		if (secondCommand == NULL) {
			if (debug) {
				printf("DEBUG: before negaMax, white pawns = "
				       "%lx\n",
				       t->piezas[blancas][peon]);
			}
			moveScore bestMove = negaMax(t, colorToMove, -1);
			printf("bestmove %s\n", moveToStr(&bestMove.move));
		} else {
			while (secondCommand != NULL) {
				for (int i = 0; i <= 7; i++) {
					if (strcmp(possible2ndCommands[i], secondCommand) == 0) {
						char * valueStr = strtok(NULL, " \t\n\r\f\v");
						switch (i) {
						case 0:
							parameters.wtime = atoi(valueStr);
							break;
						case 1:
							parameters.btime = atoi(valueStr);
							break;
						case 2:
							parameters.winc = atoi(valueStr);
							break;
						case 3:
							parameters.binc = atoi(valueStr);
							break;
						case 4:
							parameters.movestogo = atoi(valueStr);
							break;
						case 5:

							parameters.depth = atoi(valueStr);
							break;
						case 6:
							parameters.movetime = atoi(valueStr);
							break;
						case 7:
							parameters.infinite = true;
							break;
						}
					}
				}
				secondCommand = strtok(NULL, " \t\n\r\f\v");
			}
			if (parameters.depth != -1) {
				moveScore bestMove = negaMaxFixedDepth(t, colorToMove, parameters.depth);
				printf("bestmove %s\n", moveToStr(&bestMove.move));

			} else if (parameters.movetime != -1) {
				moveScore bestMove = negaMax(t, colorToMove, parameters.movetime);
				printf("bestmove %s\n", moveToStr(&bestMove.move));
			} else if (parameters.infinite) {
				moveScore bestMove = negaMax(t, colorToMove, -1);
				printf("bestmove %s\n", moveToStr(&bestMove.move));
			} else if (parameters.wtime != -1 && parameters.btime != -1) {
				int clockTime = 0;
				int increment = 0;
				if (colorToMove == blancas) {
					clockTime = parameters.wtime;
					if (parameters.winc != -1) {
						increment = parameters.winc;
					}
				} else if (colorToMove == negras) {
					clockTime = parameters.btime;
					if (parameters.binc != -1) {
						increment = parameters.binc;
					}
				}
				int timeForThisMove =
				    clockTime / (parameters.movestogo == -1 ? parameters.movestogo : 40) + increment;
				timeForThisMove -= ((increment / timeForThisMove) * 100);
				if (timeForThisMove < 100) {
					timeForThisMove = 100;
				}
				moveScore bestMove = negaMax(t, colorToMove, timeForThisMove);
				printf("bestmove %s\n", moveToStr(&bestMove.move));
			}
		}
	}
}
static inline tipoDePieza charToPiece(char c) {
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
static inline casilla stringToSq(const char * sq) {
	int file = tolower(sq[0]) - 'a';
	int rank = sq[1] - '1';
	return rank * 8 + file;
}
static inline char pieceToChar(tipoDePieza piece) {
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
}
static inline char * moveToStr(Move * move) {
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
static inline bool isEqualMoves(Move * x, Move * y) {
	if ((x->to == y->to) && (x->from == y->from) && (x->piece == y->piece) && (x->promoPiece == y->promoPiece)) {
		return true;
	} else {
		return false;
	}
}
moveSort scoreMoveForSorting(Move * move, int depth) {
	int score = 0;
	if (move->capture != -1) {
		score += 10000 + (sortingValues[move->capture] - sortingValues[move->piece]);
	} else {
		if (debug) {
			printf("DEBUG: from=%d, to=%d piece: %d\n", move->from, move->to, move->piece);
			fflush(stdout);
		}
		score += history[move->from][move->to];
		if (isEqualMoves(move, &killerMoves[depth][0]) || isEqualMoves(move, &killerMoves[depth][1])) {
			score += 8000;
		}
	}
	if (move->special == 3) {
		score += 15000 + sortingValues[move->promoPiece];
	}
	moveSort x = {score, *move};
	return x;
}
static inline void moveToMoveSort(moveLists * input, moveSort output[], int depth) {
	for (int i = 0; i < input->count; i++) {
		output[i] = scoreMoveForSorting(&input->moves[i], depth);
	}
}
static inline void swapMoveSort(moveSort * a, moveSort * b) {
	moveSort temp = *a;
	*a = *b;
	*b = temp;
}
static inline int partition(moveSort arr[], int low, int high) {

	// Choose the pivot
	int pivot = arr[high].sortingScore;

	// Index of smaller element and indicates
	// the right position of pivot found so far
	int i = low - 1;

	// Traverse arr[low..high] and move all smaller
	// elements to the left side. Elements from low to
	// i are smaller after every iteration
	for (int j = low; j <= high - 1; j++) {
		if (arr[j].sortingScore > pivot) {
			i++;
			swapMoveSort(&arr[i], &arr[j]);
		}
	}

	// Move pivot after smaller elements and
	// return its position
	swapMoveSort(&arr[i + 1], &arr[high]);
	return i + 1;
}
static inline int compareMoveSort(const void * a, const void * b) {
	const moveSort * ma = (const moveSort *)a;
	const moveSort * mb = (const moveSort *)b;
	// Descending order: higher score first
	return mb->sortingScore - ma->sortingScore;
}
float quiescence(Tablero * t, color c, float alpha, float beta) {
	float standPat = boardEval(t, c); // current evaluation
	if (standPat >= beta) {
		return beta;
	}
	if (standPat > alpha) {
		alpha = standPat;
	}
	moveLists captures = {0};
	generateCaptures(c, t, &captures);
	moveSort moves[256];
	moveToMoveSort(&captures, moves, 0);
	qsort(moves, captures.count, sizeof(moveSort), compareMoveSort);

	for (int i = 0; i < captures.count; i++) {
		Tablero temp = *t;
		makeMove(&captures.moves[i], &temp, c);
		float score = -quiescence(&temp, !c, -beta, -alpha);
		if (score >= beta)
			return beta;
		if (score > alpha)
			alpha = score;
	}
	return alpha;
}
bitboard attackedByColor(Tablero * t, color attacker) {
	bitboard attacked = 0;

	// Knights
	bitboard knights = t->piezas[attacker][caballo];
	while (knights) {
		int sq = __builtin_ctzll(knights);
		knights &= knights - 1;
		attacked |= knightAttacks[sq];
	}

	// Kings
	bitboard kings = t->piezas[attacker][rey];
	while (kings) {
		int sq = __builtin_ctzll(kings);
		kings &= kings - 1;
		attacked |= kingAttacks[sq];
	}

	// Pawns
	bitboard pawns = t->piezas[attacker][peon];
	while (pawns) {
		int sq = __builtin_ctzll(pawns);
		pawns &= pawns - 1;
		attacked |= pawnAttacks[attacker][sq];
	}

	// Rooks and Queens (orthogonal)
	bitboard orthPieces = t->piezas[attacker][torre] | t->piezas[attacker][reina];
	while (orthPieces) {
		int sq = __builtin_ctzll(orthPieces);
		orthPieces &= orthPieces - 1;
		for (int d = 0; d < 4; d++) {
			bitboard ray = rookMask[sq][d];
			bitboard blockers = ray & t->allOccupiedSquares;
			if (blockers) {
				int firstBlocker;
				if (d < 2) // north/east (positive)
					firstBlocker = __builtin_ctzll(blockers);
				else // south/west (negative)
					firstBlocker = 63 - __builtin_clzll(blockers);

				// Squares from the piece up to but not including the blocker
				bitboard before;
				if (d < 2)
					before = ray & ((1ULL << firstBlocker) - 1);
				else
					before = ray & ~((1ULL << (firstBlocker + 1)) - 1);

				attacked |= before;

				// Include the blocker if it's an enemy piece (since it can be captured)
				if (BB_SQUARE(firstBlocker) & t->allPieces[!attacker]) {
					attacked |= BB_SQUARE(firstBlocker);
				}
			} else {
				// No blockers: entire ray is attacked
				attacked |= ray;
			}
		}
	}

	// Bishops and Queens (diagonal)
	bitboard diaPieces = t->piezas[attacker][alfil] | t->piezas[attacker][reina];
	while (diaPieces) {
		int sq = __builtin_ctzll(diaPieces);
		diaPieces &= diaPieces - 1;
		for (int d = 0; d < 4; d++) {
			bitboard ray = bishopMask[sq][d];
			bitboard blockers = ray & t->allOccupiedSquares;
			if (blockers) {
				int firstBlocker;
				if (d > 1) // down-left/down-right (positive)
					firstBlocker = __builtin_ctzll(blockers);
				else // up-left/up-right (negative)
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
void initZobrist() {
	for (int i = 0; i < 2; i++) {
		for (int j = 0; j < 6; j++) {
			for (int k = i; k < 64; k++) {
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
