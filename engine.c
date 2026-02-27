#define _POSIX_C_SOURCE 199309L
#include <ctype.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
typedef uint64_t bitboard;
// macros
#define C64(constantU64) constantU64##ULL
#define BB_SQUARE(sq) (1ULL << (sq))
#define RANK(r) (C64(0xFF) << (8 * (r)))
#define C8 ((uint8_t)123)
#define MAX_DEPTH 128
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
#define TT_SIZE (1 << 20)
#define TT_MASK (TT_SIZE - 1)

typedef struct {
	uint64_t key; // full Zobrist hash to verify entry
	int depth;    // remaining depth at which this position was searched
	float score;  // evaluation (from perspective of side to move)
	uint8_t flag; // 0 = exact, 1 = lower bound (beta cutoff), 2 = upper bound (fail low)
} TTEntry;

TTEntry tt[TT_SIZE];
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
     {0,  0,  0,  0,  0,  0,  0,  0,  50, 50, 50, 50, 50, 50, 50, 50, 30, 30, 35, 40, 40, 35,
      30, 30, 20, 20, 25, 30, 30, 25, 20, 20, 10, 10, 15, 20, 20, 15, 10, 10, 5,  5,  10, 10,
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
     // rey (medio juego)
     {-30, -40, -40, -50, -50, -40, -40, -30, -30, -40, -40, -50, -50, -40, -40, -30, -30, -40, -40, -50, -50, -40,
      -40, -30, -30, -40, -40, -50, -50, -40, -40, -30, -20, -30, -30, -40, -40, -30, -30, -20, -10, -20, -20, -20,
      -20, -20, -20, -10, 20,  20,  0,	 0,   0,   0,	20,  20,  20,  30,  10,	 0,   0,   10,	30,  20}},
    // negras (espejo vertical)
    {// peon
     {0,  0,  0,  0,  0,  0,  0,  0,  50, 50, 50, 50, 50, 50, 50, 50, 30, 30, 35, 40, 40, 35,
      30, 30, 20, 20, 25, 30, 30, 25, 20, 20, 10, 10, 15, 20, 20, 15, 10, 10, 5,  5,  10, 10,
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
     {-20, -10, -10, -5,  -5,  -10, -10, -20, -10, 0,  0, 0,   0,   0,	 0,   -10, -10, 0,   5,	  5,  5, 5,
      0,   -10, -5,  0,	  5,   5,   5,	 5,   0,   -5, 0, 0,   5,   5,	 5,   5,   0,	-5,  -10, 5,  5, 5,
      5,   5,	0,   -10, -10, 0,   5,	 0,   0,   0,  0, -10, -20, -10, -10, -5,  -5,	-10, -10, -20},
     // rey (medio juego)
     {-30, -40, -40, -50, -50, -40, -40, -30, -30, -40, -40, -50, -50, -40, -40, -30, -30, -40, -40, -50, -50, -40,
      -40, -30, -30, -40, -40, -50, -50, -40, -40, -30, -20, -30, -30, -40, -40, -30, -30, -20, -10, -20, -20, -20,
      -20, -20, -20, -10, 20,  20,  0,	 0,   0,   0,	20,  20,  20,  30,  10,	 0,   0,   10,	30,  20}}};
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
float boardEval(Tablero * t, color c, int myMoveCount);
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
float quiescence(Tablero * t, color c, float alpha, float beta, int qdepth);
bitboard attackedByColor(Tablero * t, color attacker);
void unmakeMove(Move * move, Tablero * t, color c, int oldEnPassant, uint8_t oldCastling, int oldHalfClock,
		int oldFullClock);
void initZobrist();
uint64_t computeZobrist(Zobrist * z, Tablero * t, color sideToMove);
void testZobrist();
void testTT();
void generateAllPseudoMoves(color c, Tablero * t, moveLists * output);
void generateRookCaptures(moveLists * ml, color c, Tablero * t);
void generateBishopCaptures(moveLists * ml, color c, Tablero * t);
void generateKnightCaptures(moveLists * ml, color c, Tablero * t);
void generatePawnCaptures(moveLists * ml, color c, Tablero * t);
void generateQueenCaptures(moveLists * ml, color c, Tablero * t);
void generateKingCaptures(moveLists * ml, color c, Tablero * t);
bool testUnmakeMove(Tablero * original, Move move, color c);
void testUnmakeAll();
static inline bitboard BB_FILE(int file);
int main() {
	setbuf(stdout, NULL); // Desactiva el buffering de stdout
	setbuf(stdin, NULL);
	initAttackTables();
	initZobrist();
	/*
	testZobrist();
	testTT();
	testUnmakeAll();
	*/
	while (true) {
		if (inputAvaliable()) {
			char buffer[4096];
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

static long long int TThits = 0, TTmisses = 0;
float recursiveNegaMax(int depth, Tablero * t, color c, float alpha, float beta) {
	nodes++;
	int index = t->hash & TT_MASK;
	TTEntry * entry = &tt[index];
	if (entry->key == t->hash && entry->depth >= depth) {
		// We have a stored result from at least this depth
		TThits++;
		if (entry->flag == 0)
			return entry->score; // exact score
		if (entry->flag == 1 && entry->score >= beta)
			return beta; // lower bound cutoff
		if (entry->flag == 2 && entry->score <= alpha)
			return alpha; // upper bound cutoff
		// Otherwise, we can't use the score directly, but we might still use the best move for ordering
	} else {
		TTmisses++;
	}
	float oldAlpha = alpha;
	if (debug) {
		printf("DEBUG: recursiveNegaMax start, colorToMove = %d\n", c);
	}
	moveLists colorToMove = {0};
	generateAllPseudoMoves(c, t, &colorToMove);
	if (depth == 0 || colorToMove.count == 0) {
		return quiescence(t, c, alpha, beta, 4);
	}
	if (nodes % 1024 == 0) {
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
		casilla oldEp = t->enPassantSquare;
		uint8_t oldCastling = t->castlingRights;
		int oldHalf = t->halfmoveClock;
		int oldFull = t->fullMoves;
		Move move = moves[i].move;
		makeMove(&move, t, c);
		casilla king = __builtin_ctzll(t->piezas[c][rey]);
		if (isAttacked(t, king, !c)) {
			unmakeMove(&move, t, c, oldEp, oldCastling, oldHalf, oldFull);
			continue;
		}
		float score = -recursiveNegaMax(depth - 1, t, !c, -beta, -alpha);
		unmakeMove(&move, t, c, oldEp, oldCastling, oldHalf, oldFull);
		if (score > alpha) {
			alpha = score;
		}
		if (alpha >= beta) {
			if (moves[i].move.capture == 0 && moves[i].move.capture != 3) {
				history[moves[i].move.from][moves[i].move.to] += depth * depth;
				if (depth < MAX_DEPTH) {
					if (isEqualMoves(&move, &killerMoves[depth][0])) {
						continue;
					} else {
						killerMoves[depth][1] = killerMoves[depth][0];
						killerMoves[depth][0] = move;
					}
				}
			}
			break;
		}
	}
	uint8_t flag;
	if (alpha <= oldAlpha)
		flag = 2; // fail low – upper bound
	else if (alpha >= beta)
		flag = 1; // fail high – lower bound
	else
		flag = 0; // exact

	// Replace if deeper or slot is empty
	if (entry->depth <= depth || entry->key == 0) {
		entry->key = t->hash;
		entry->depth = depth;
		entry->score = alpha;
		entry->flag = flag;
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
		    .score = boardEval(t, c, -1),
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
			if (nodes % 1024 == 0) {
				if (inputAvaliable()) {
					char buffer[256];
					if (fgets(buffer, sizeof(buffer), stdin)) {
						proccesUCICommands(buffer, t);
					}
				}
			}
			casilla oldEp = t->enPassantSquare;
			uint8_t oldCastling = t->castlingRights;
			int oldHalf = t->halfmoveClock;
			int oldFull = t->fullMoves;
			makeMove(&colorToMove.moves[i], t, c);
			float score = -recursiveNegaMax(depth, t, !c, -beta, -alpha);
			unmakeMove(&colorToMove.moves[i], t, c, oldEp, oldCastling, oldHalf, oldFull);
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
	if (debug) {
		printf("TT probes: %lld, hits: %lld, hit rate: %.2f%%\n", TTmisses, TThits, 100.0 * TThits / TTmisses);
	}
	return bm;
}
moveScore negaMaxFixedDepth(Tablero * t, color c, int depth) {
	moveLists colorToMove = {0};
	generateAllMoves(c, t, &colorToMove);
	memset(history, 0, sizeof(history));
	if (colorToMove.count == 0) {
		moveScore output = {.move = {0}, .score = boardEval(t, c, colorToMove.count)};
		return output;
	}

	float bestScore = -INFINITY;
	Move bestMove = {0};
	float alpha = -INFINITY;
	float beta = INFINITY;

	for (int i = 0; i < colorToMove.count; i++) {
		if (nodes % 1024 == 0) {
			if (inputAvaliable()) {
				char buffer[256];
				if (fgets(buffer, sizeof(buffer), stdin)) {
					proccesUCICommands(buffer, t);
				}
			}
		}
		casilla oldEp = t->enPassantSquare;
		uint8_t oldCastling = t->castlingRights;
		int oldHalf = t->halfmoveClock;
		int oldFull = t->fullMoves;
		makeMove(&colorToMove.moves[i], t, c);
		float score = -recursiveNegaMax(depth - 1, t, !c, -beta, -alpha);
		unmakeMove(&colorToMove.moves[i], t, c, oldEp, oldCastling, oldHalf, oldFull);

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
	if (debug) {
		printf("TT probes: %lld, hits: %lld, hit rate: %.2f%%\n", TTmisses, TThits, 100.0 * TThits / TTmisses);
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
void generateCaptures(color c, Tablero * t, moveLists * output) {
	output->count = 0;
	generatePawnCaptures(output, c, t);
	generateKnightCaptures(output, c, t);
	generateBishopCaptures(output, c, t);
	generateRookCaptures(output, c, t);
	generateQueenCaptures(output, c, t);
	generateKingCaptures(output, c, t);
	// No legality test here – quiescence will test after making the move.
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
	if (king == 0)
		return;
	if (king & (king - 1))
		return; // multiple kings (shouldn't happen)
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
				      BB_SQUARE(g1) & attacked)) { // is the square we are  moving to attacked
								   // || is the king in check
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
		// White en passant
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
		// Black en passant
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
float boardEval(Tablero * t, color c, int myMoveCount) {
	if (t->piezas[c][rey] == 0)
		return 0;

	float value =
	    100 * (__builtin_popcountll(t->piezas[c][peon]) - __builtin_popcountll(t->piezas[!c][peon])) +
	    300 * (__builtin_popcountll(t->piezas[c][caballo]) - __builtin_popcountll(t->piezas[!c][caballo])) +
	    350 * (__builtin_popcountll(t->piezas[c][alfil]) - __builtin_popcountll(t->piezas[!c][alfil])) +
	    500 * (__builtin_popcountll(t->piezas[c][torre]) - __builtin_popcountll(t->piezas[!c][torre])) +
	    900 * (__builtin_popcountll(t->piezas[c][reina]) - __builtin_popcountll(t->piezas[!c][reina]));

	float positional = 0;
	for (tipoDePieza p = peon; p <= rey; p++) {
		bitboard pieces = t->piezas[c][p];
		while (pieces) {
			int sq = __builtin_ctzll(pieces);
			pieces &= pieces - 1;
			positional += positionalValues[c][p][sq];
		}
	}
	for (tipoDePieza p = peon; p <= rey; p++) {
		bitboard pieces = t->piezas[!c][p];
		while (pieces) {
			int sq = __builtin_ctzll(pieces);
			pieces &= pieces - 1;
			positional -= positionalValues[!c][p][sq];
		}
	}
	value += (10 * positional);

	int myMobility = myMoveCount;
	if (myMobility == -1) {
		moveLists moves;
		generateAllPseudoMoves(c, t, &moves);
		myMobility = moves.count;
	}
	moveLists oppMoves;
	generateAllPseudoMoves(!c, t, &oppMoves);
	int oppMobility = oppMoves.count;

	value += 5 * (myMobility - oppMobility);

	int kingSq = __builtin_ctzll(t->piezas[c][rey]);
	bitboard kingZone = kingAttacks[kingSq] | BB_SQUARE(kingSq);
	int attackers = __builtin_popcountll(kingZone & t->allPieces[!c]);
	value -= attackers * 50;

	int development = 0;
	if (!(t->piezas[c][caballo] & BB_SQUARE(b1)) && c == blancas)
		development += 20;
	if (!(t->piezas[c][caballo] & BB_SQUARE(g1)) && c == blancas)
		development += 20;
	if (!(t->piezas[c][alfil] & BB_SQUARE(c1)) && c == blancas)
		development += 20;
	if (!(t->piezas[c][alfil] & BB_SQUARE(f1)) && c == blancas)
		development += 20;

	if (!(t->piezas[!c][caballo] & BB_SQUARE(b8)) && c == blancas)
		development -= 20;
	if (!(t->piezas[!c][caballo] & BB_SQUARE(g8)) && c == blancas)
		development -= 20;
	if (!(t->piezas[!c][alfil] & BB_SQUARE(c8)) && c == blancas)
		development -= 20;
	if (!(t->piezas[!c][alfil] & BB_SQUARE(f8)) && c == blancas)
		development -= 20;

	if ((t->castlingRights & (WHITE_OO | WHITE_OOO)) && c == blancas)
		development -= 10;
	if ((t->castlingRights & (BLACK_OO | BLACK_OOO)) && c == blancas)
		development += 10;
	value += development;

	int pawnStruct = 0;
	for (int file = 0; file < 8; file++) {
		int countOwn = __builtin_popcountll(t->piezas[c][peon] & BB_FILE(file));
		if (countOwn > 1)
			pawnStruct -= (countOwn - 1) * 15;

		int countOpp = __builtin_popcountll(t->piezas[!c][peon] & BB_FILE(file));
		if (countOpp > 1)
			pawnStruct += (countOpp - 1) * 15;
	}
	value += pawnStruct;

	int rookBonus = 0;
	for (int file = 0; file < 8; file++) {
		bitboard fileMask = BB_FILE(file);
		if (!(t->piezas[c][peon] & fileMask)) {
			if (t->piezas[c][torre] & fileMask)
				rookBonus += 30;
		}
		if (!(t->piezas[!c][peon] & fileMask)) {
			if (t->piezas[!c][torre] & fileMask)
				rookBonus -= 30;
		}
	}
	value += rookBonus;

	return value;
}
void makeMove(Move * move, Tablero * t, color c) {
	casilla enPassantSq = t->enPassantSquare;
	uint8_t oldCastling = t->castlingRights; // save before any changes

	// XOR out old en passant file (if any)
	if (t->enPassantSquare != -1) {
		int file = t->enPassantSquare % 8;
		t->hash ^= zobrist.enPassant[file];
	}
	t->enPassantSquare = -1;

	t->fullMoves++;
	if (move->piece == peon)
		t->halfmoveClock = 0;
	else
		t->halfmoveClock++;

	switch (move->special) {
		case 0: {
			// Remove moving piece from old square
			t->piezas[c][move->piece] &= ~(C64(1) << move->from);
			t->allPieces[c] &= ~(BB_SQUARE(move->from));
			t->allOccupiedSquares &= ~(BB_SQUARE(move->from));
			t->hash ^= zobrist.pieces[c][move->piece][move->from];

			// Handle capture
			if (move->capture != -1) {
				t->piezas[!c][move->capture] &= ~(C64(1) << move->to);
				t->allPieces[!c] &= ~(BB_SQUARE(move->to));
				t->allOccupiedSquares &= ~(BB_SQUARE(move->to));
				t->hash ^= zobrist.pieces[!c][move->capture][move->to];
			}

			// Add moving piece to new square
			t->piezas[c][move->piece] |= (C64(1) << move->to);
			t->allPieces[c] |= BB_SQUARE(move->to);
			t->allOccupiedSquares |= BB_SQUARE(move->to);
			t->hash ^= zobrist.pieces[c][move->piece][move->to];

			// Update castling rights based on old rights
			// King move
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
			// Rook move from original square
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
			// Rook capture
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

			// Set en passant square if double pawn push
			if (move->piece == peon && abs(move->to - move->from) == 16) {
				if (c == blancas)
					t->enPassantSquare = move->to - 8;
				else
					t->enPassantSquare = move->to + 8;
				// XOR in new en passant file
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

			// Remove capturing pawn from old square
			t->piezas[c][move->piece] &= ~(C64(1) << move->from);
			t->allPieces[c] &= ~(BB_SQUARE(move->from));
			t->allOccupiedSquares &= ~(BB_SQUARE(move->from));
			t->hash ^= zobrist.pieces[c][move->piece][move->from];

			// Remove captured pawn
			t->piezas[!c][peon] &= ~(C64(1) << opponentPawn);
			t->allPieces[!c] &= ~(BB_SQUARE(opponentPawn));
			t->allOccupiedSquares &= ~(BB_SQUARE(opponentPawn));
			t->hash ^= zobrist.pieces[!c][peon][opponentPawn];

			// Add capturing pawn to new square
			t->piezas[c][move->piece] |= (C64(1) << move->to);
			t->allPieces[c] |= BB_SQUARE(move->to);
			t->allOccupiedSquares |= BB_SQUARE(move->to);
			t->hash ^= zobrist.pieces[c][move->piece][move->to];

			break;
		}

		case 2: {
			// Move king
			t->piezas[c][rey] &= ~(C64(1) << move->from);
			t->piezas[c][rey] |= (C64(1) << move->to);
			t->allPieces[c] &= ~(BB_SQUARE(move->from));
			t->allPieces[c] |= BB_SQUARE(move->to);
			t->allOccupiedSquares &= ~(BB_SQUARE(move->from));
			t->allOccupiedSquares |= BB_SQUARE(move->to);
			t->hash ^= zobrist.pieces[c][rey][move->from];
			t->hash ^= zobrist.pieces[c][rey][move->to];

			// Move rook based on destination
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

			// Update castling rights based on old rights
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
			// Remove pawn from old square
			t->piezas[c][peon] &= ~(C64(1) << move->from);
			t->allPieces[c] &= ~(BB_SQUARE(move->from));
			t->allOccupiedSquares &= ~(BB_SQUARE(move->from));
			t->hash ^= zobrist.pieces[c][peon][move->from];

			// Handle capture
			if (move->capture != -1) {
				t->piezas[!c][move->capture] &= ~(C64(1) << move->to);
				t->allPieces[!c] &= ~(BB_SQUARE(move->to));
				t->allOccupiedSquares &= ~(BB_SQUARE(move->to));
				t->hash ^= zobrist.pieces[!c][move->capture][move->to];
			}

			// Add promoted piece to new square
			t->piezas[c][move->promoPiece] |= (C64(1) << move->to);
			t->allPieces[c] |= BB_SQUARE(move->to);
			t->allOccupiedSquares |= BB_SQUARE(move->to);
			t->hash ^= zobrist.pieces[c][move->promoPiece][move->to];

			break;
		}
	}

	// Toggle side to move
	t->hash ^= zobrist.side;
}
void unmakeMove(Move * move, Tablero * t, color c, int oldEnPassant, uint8_t oldCastling, int oldHalfClock,
		int oldFullClock) {

	// Restore clocks (no hash impact)
	t->halfmoveClock = oldHalfClock;
	t->fullMoves = oldFullClock;

	// Undo piece movements according to move type
	switch (move->special) {
		case 0: {
			// Remove moving piece from destination square
			t->piezas[c][move->piece] &= ~BB_SQUARE(move->to);
			t->allPieces[c] &= ~BB_SQUARE(move->to);
			t->allOccupiedSquares &= ~BB_SQUARE(move->to);
			t->hash ^= zobrist.pieces[c][move->piece][move->to];

			// If there was a capture, restore captured piece on destination
			if (move->capture != -1) {
				t->piezas[!c][move->capture] |= BB_SQUARE(move->to);
				t->allPieces[!c] |= BB_SQUARE(move->to);
				t->allOccupiedSquares |= BB_SQUARE(move->to);
				t->hash ^= zobrist.pieces[!c][move->capture][move->to];
			}

			// Put moving piece back on original square
			t->piezas[c][move->piece] |= BB_SQUARE(move->from);
			t->allPieces[c] |= BB_SQUARE(move->from);
			t->allOccupiedSquares |= BB_SQUARE(move->from);
			t->hash ^= zobrist.pieces[c][move->piece][move->from];
			break;
		}

		case 1: {
			// Remove capturing pawn from destination
			t->piezas[c][move->piece] &= ~BB_SQUARE(move->to);
			t->allPieces[c] &= ~BB_SQUARE(move->to);
			t->allOccupiedSquares &= ~BB_SQUARE(move->to);
			t->hash ^= zobrist.pieces[c][move->piece][move->to];

			// Put capturing pawn back on original
			t->piezas[c][move->piece] |= BB_SQUARE(move->from);
			t->allPieces[c] |= BB_SQUARE(move->from);
			t->allOccupiedSquares |= BB_SQUARE(move->from);
			t->hash ^= zobrist.pieces[c][move->piece][move->from];

			// Restore captured pawn using old en passant square
			casilla opponentPawn = (c == blancas ? oldEnPassant - 8 : oldEnPassant + 8);
			t->piezas[!c][peon] |= BB_SQUARE(opponentPawn);
			t->allPieces[!c] |= BB_SQUARE(opponentPawn);
			t->allOccupiedSquares |= BB_SQUARE(opponentPawn);
			t->hash ^= zobrist.pieces[!c][peon][opponentPawn];
			break;
		}

		case 2: {
			// Undo king move
			t->piezas[c][rey] &= ~BB_SQUARE(move->to);
			t->allPieces[c] &= ~BB_SQUARE(move->to);
			t->allOccupiedSquares &= ~BB_SQUARE(move->to);
			t->hash ^= zobrist.pieces[c][rey][move->to];

			t->piezas[c][rey] |= BB_SQUARE(move->from);
			t->allPieces[c] |= BB_SQUARE(move->from);
			t->allOccupiedSquares |= BB_SQUARE(move->from);
			t->hash ^= zobrist.pieces[c][rey][move->from];

			// Determine rook's original and destination squares (reverse of makeMove)
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

			// Undo rook move
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
			// Remove promoted piece from destination
			t->piezas[c][move->promoPiece] &= ~BB_SQUARE(move->to);
			t->allPieces[c] &= ~BB_SQUARE(move->to);
			t->allOccupiedSquares &= ~BB_SQUARE(move->to);
			t->hash ^= zobrist.pieces[c][move->promoPiece][move->to];

			// If there was a capture, restore captured piece on destination
			if (move->capture != -1) {
				t->piezas[!c][move->capture] |= BB_SQUARE(move->to);
				t->allPieces[!c] |= BB_SQUARE(move->to);
				t->allOccupiedSquares |= BB_SQUARE(move->to);
				t->hash ^= zobrist.pieces[!c][move->capture][move->to];
			}

			// Restore pawn on original square
			t->piezas[c][peon] |= BB_SQUARE(move->from);
			t->allPieces[c] |= BB_SQUARE(move->from);
			t->allOccupiedSquares |= BB_SQUARE(move->from);
			t->hash ^= zobrist.pieces[c][peon][move->from];
			break;
		}
	}

	// --- Restore castling rights and update hash ---
	uint8_t currentRights = t->castlingRights;
	if (currentRights & WHITE_OO)
		t->hash ^= zobrist.castling[0];
	if (currentRights & WHITE_OOO)
		t->hash ^= zobrist.castling[1];
	if (currentRights & BLACK_OO)
		t->hash ^= zobrist.castling[2];
	if (currentRights & BLACK_OOO)
		t->hash ^= zobrist.castling[3];

	if (oldCastling & WHITE_OO)
		t->hash ^= zobrist.castling[0];
	if (oldCastling & WHITE_OOO)
		t->hash ^= zobrist.castling[1];
	if (oldCastling & BLACK_OO)
		t->hash ^= zobrist.castling[2];
	if (oldCastling & BLACK_OOO)
		t->hash ^= zobrist.castling[3];

	t->castlingRights = oldCastling;

	// --- Restore en passant square and update hash ---
	if (t->enPassantSquare != -1) {
		int file = t->enPassantSquare % 8;
		t->hash ^= zobrist.enPassant[file];
	}
	if (oldEnPassant != -1) {
		int file = oldEnPassant % 8;
		t->hash ^= zobrist.enPassant[file];
	}
	t->enPassantSquare = oldEnPassant;

	// Toggle side to move back (undo the side change)
	t->hash ^= zobrist.side;
}
bool inputAvaliable() {
	struct timeval tv = {0, 0};
	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(0, &fds);
	int ret = select(1, &fds, NULL, NULL, &tv);
	return (ret > 0);
}
void proccesUCICommands(char command[4096], Tablero * t) {
	char * nl = strchr(command, '\n');
	if (nl)
		*nl = '\0';

	if (strcmp(command, "quit") == 0) {
		exit(0);
	}
	if (strcmp(command, "ping") == 0) {
		printf("pong\n");
		fflush(stdout);
		return;
	}
	if (strcmp(command, "stop") == 0) {
		stopRequested = true;
		return;
	}
	if (strcmp(command, "uci") == 0) {
		printf("id name Supplycat\n");
		printf("id author  arkar\n");
		printf("uciok\n");
		fflush(stdout);
		return;
	}
	if (strcmp(command, "isready") == 0) {
		printf("readyok\n");
		fflush(stdout);
		return;
	}
	if (strcmp(command, "ucinewgame") == 0) {
		memset(tt, 0, sizeof(tt));
		memset(history, 0, sizeof(history));
		memset(killerMoves, 0, sizeof(killerMoves));
		return;
	}

	// Comandos que pueden tener parámetros – usamos strtok
	char * firstCommand = strtok(command, " \t\n\r\f\v");
	if (!firstCommand)
		return; // línea vacía

	if (debug) {
		printf("DEBUG: firstCommand = '%s'\n", firstCommand);
		fflush(stdout);
	}

	if (strcmp(firstCommand, "debug") == 0) {
		char * x = strtok(NULL, " \t\n\r\f\v");
		if (x && strcmp(x, "off") == 0) {
			debug = false;
			printf("Debug off\n");
		} else if (x && strcmp(x, "on") == 0) {
			debug = true;
			printf("Debug on\n");
		}
		fflush(stdout);
		return;
	}

	if (strcmp(firstCommand, "position") == 0) {
		char * secondCommand = strtok(NULL, " \t\n\r\f\v");
		if (!secondCommand)
			return;
		if (debug) {
			printf("DEBUG: secondCommand = '%s'\n", secondCommand);
			fflush(stdout);
		}

		if (strcmp(secondCommand, "startpos") == 0) {
			initBoard(t);
			colorToMove = blancas;
			char * movesToken = strtok(NULL, " ");
			if (movesToken && strcmp(movesToken, "moves") == 0) {
				char * move = strtok(NULL, " ");
				while (move) {
					// Parseo del movimiento (igual que tenías)
					casilla to = stringToSq(move + 2);
					casilla from = stringToSq(move);
					tipoDePieza promo = 0;
					tipoDePieza piece = -1;
					int special = 0;
					tipoDePieza capture = -1;

					for (int i = peon; i <= rey; i++) {
						if (BB_SQUARE(from) & t->piezas[colorToMove][i]) {
							piece = i;
							break;
						}
					}
					if (piece == -1) {
						fprintf(stderr, "ERROR: INVALID MOVE\n");
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
						capture = peon;
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
			if (debug)
				printBitboard(t->allOccupiedSquares);
			return;
		}

		if (strcmp(secondCommand, "fen") == 0) {
			memset(t, 0, sizeof(Tablero));
			char * fenString = strtok(NULL, " \t\n\r\f\v");
			if (!fenString)
				return;
			if (debug)
				printf("DEBUG: fenString = '%s'\n", fenString);
			int rank = 7;
			int file = 0;
			for (int i = 0; fenString[i]; i++) {
				char currentChar = fenString[i];
				if (currentChar == '/') {
					rank--;
					file = 0;
					continue;
				}
				if (isdigit(currentChar)) {
					file += currentChar - '0';
					continue;
				}
				color c = isupper(currentChar) ? blancas : negras;
				tipoDePieza piece = charToPiece(currentChar);
				casilla square = rank * 8 + file;
				t->piezas[c][piece] |= BB_SQUARE(square);
				file++;
				if (debug)
					printf("Placing %c at square %d\n", currentChar, square);
			}
			updateBoardCache(t);
			char * activeColor = strtok(NULL, " ");
			colorToMove = strcmp(activeColor, "w") ? negras : blancas;
			char * castlingRights = strtok(NULL, " ");
			t->castlingRights = 0;
			if (strchr(castlingRights, 'K'))
				t->castlingRights |= WHITE_OO;
			if (strchr(castlingRights, 'Q'))
				t->castlingRights |= WHITE_OOO;
			if (strchr(castlingRights, 'k'))
				t->castlingRights |= BLACK_OO;
			if (strchr(castlingRights, 'q'))
				t->castlingRights |= BLACK_OOO;
			char * enPassantSquare = strtok(NULL, " ");
			if (strchr(enPassantSquare, '-')) {
				t->enPassantSquare = -1;
			} else {
				t->enPassantSquare = stringToSq(enPassantSquare);
			}
			char * halfmoveClock = strtok(NULL, " ");
			t->halfmoveClock = atoi(halfmoveClock);
			char * fullMove = strtok(NULL, " ");
			t->fullMoves = atoi(fullMove);
			char * movesToken = strtok(NULL, " ");
			if (movesToken && strcmp(movesToken, "moves") == 0) {
				char * move = strtok(NULL, " ");
				while (move) {
					// (mismo parseo que en startpos)
					casilla to = stringToSq(move + 2);
					casilla from = stringToSq(move);
					tipoDePieza promo = 0;
					tipoDePieza piece = -1;
					int special = 0;
					tipoDePieza capture = -1;

					for (int i = peon; i <= rey; i++) {
						if (BB_SQUARE(from) & t->piezas[colorToMove][i]) {
							piece = i;
							break;
						}
					}
					if (piece == -1) {
						fprintf(stderr, "ERROR: INVALID MOVE\n");
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
						capture = peon;
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
			return;
		}
	}

	if (strcmp(firstCommand, "go") == 0) {
		// Reiniciamos parámetros
		parameters = (goParameters){.wtime = -1,
					    .btime = -1,
					    .winc = -1,
					    .binc = -1,
					    .movestogo = -1,
					    .depth = -1,
					    .movetime = -1,
					    .infinite = false};
		char * token = strtok(NULL, " \t\n\r\f\v");
		char * possible2ndCommands[8] = {"wtime",     "btime", "winc",	   "binc",
						 "movestogo", "depth", "movetime", "infinite"};

		while (token) {
			for (int i = 0; i < 8; i++) {
				if (strcmp(token, possible2ndCommands[i]) == 0) {
					if (i == 7) { // infinite
						parameters.infinite = true;
						break;
					}
					char * valueStr = strtok(NULL, " \t\n\r\f\v");
					if (!valueStr)
						break;
					int val = atoi(valueStr);
					switch (i) {
						case 0:
							parameters.wtime = val;
							break;
						case 1:
							parameters.btime = val;
							break;
						case 2:
							parameters.winc = val;
							break;
						case 3:
							parameters.binc = val;
							break;
						case 4:
							parameters.movestogo = val;
							break;
						case 5:
							parameters.depth = val;
							break;
						case 6:
							parameters.movetime = val;
							break;
					}
					break;
				}
			}
			token = strtok(NULL, " \t\n\r\f\v");
		}

		moveScore bestMove;
		if (parameters.depth != -1) {
			nodes = 0; // Reinicia contador
			struct timespec start, end;
			clock_gettime(CLOCK_MONOTONIC, &start);
			bestMove = negaMaxFixedDepth(t, colorToMove, parameters.depth);
			clock_gettime(CLOCK_MONOTONIC, &end);
			double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
			double nps = nodes / elapsed;
			printf("info nodes %lld time %.0f ms nps %.0f\n", nodes, elapsed * 1000, nps);
		} else if (parameters.movetime != -1) {
			bestMove = negaMax(t, colorToMove, parameters.movetime);
		} else if (parameters.infinite) {
			bestMove = negaMax(t, colorToMove, -1);
		} else if (parameters.wtime != -1 && parameters.btime != -1) {
			int clockTime = (colorToMove == blancas) ? parameters.wtime : parameters.btime;
			int inc = (colorToMove == blancas) ? parameters.winc : parameters.binc;
			int movesLeft = (parameters.movestogo > 0) ? parameters.movestogo : 40;
			int timeForMove = clockTime / movesLeft + inc;
			if (timeForMove < 100)
				timeForMove = 100;
			bestMove = negaMax(t, colorToMove, timeForMove);
		} else {
			// Si no hay parámetros, go = infinito
			bestMove = negaMax(t, colorToMove, -1);
		}
		printf("DEBUG: from=%d to=%d special=%d promo=%d\n", bestMove.move.from, bestMove.move.to,
		       bestMove.move.special, bestMove.move.promoPiece);
		printf("bestmove %s\n", moveToStr(&bestMove.move));
		fflush(stdout);
		return;
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
		if (depth < MAX_DEPTH) {
			if (isEqualMoves(move, &killerMoves[depth][0]) || isEqualMoves(move, &killerMoves[depth][1])) {
				score += 8000;
			}
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
float quiescence(Tablero * t, color c, float alpha, float beta, int qdepth) {
	moveLists captures = {0};
	generateCaptures(c, t, &captures);
	if (qdepth <= 0) {
		return boardEval(t, c, captures.count);
	}
	float standPat = boardEval(t, c, captures.count); // current evaluation
	if (standPat >= beta) {
		return beta;
	}
	if (standPat > alpha) {
		alpha = standPat;
	}
	moveSort moves[256];
	moveToMoveSort(&captures, moves, 0);
	qsort(moves, captures.count, sizeof(moveSort), compareMoveSort);

	for (int i = 0; i < captures.count; i++) {
		casilla oldEp = t->enPassantSquare;
		uint8_t oldCastling = t->castlingRights;
		int oldHalf = t->halfmoveClock;
		int oldFull = t->fullMoves;
		makeMove(&captures.moves[i], t, c);
		casilla king = __builtin_ctzll(t->piezas[c][rey]);
		if (isAttacked(t, king, !c)) {
			unmakeMove(&captures.moves[i], t, c, oldEp, oldCastling, oldHalf, oldFull);
			continue;
		}
		float score = -quiescence(t, !c, -beta, -alpha, qdepth - 1);
		unmakeMove(&captures.moves[i], t, c, oldEp, oldCastling, oldHalf, oldFull);
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
void testZobrist() {
	Tablero t;
	initBoard(&t);
	// First sequence: e2e4, e7e5, g1f3, b8c6
	char * moves1[] = {"e2e4", "e7e5", "g1f3", "b8c6"};
	color side = blancas;
	for (int i = 0; i < 4; i++) {
		Move move;
		move.from = stringToSq(moves1[i]);
		move.to = stringToSq(moves1[i] + 2);
		move.piece = -1;
		for (int p = peon; p <= rey; p++) {
			if (BB_SQUARE(move.from) & t.piezas[side][p]) {
				move.piece = p;
				break;
			}
		}
		move.capture = -1;
		move.special = 0;
		move.promoPiece = 0;
		makeMove(&move, &t, side);
		side = !side;
	}
	uint64_t hash1 = t.hash;

	// Second sequence: g1f3, e7e5, e2e4, b8c6
	initBoard(&t);
	char * moves2[] = {"g1f3", "e7e5", "e2e4", "b8c6"};
	side = blancas;
	for (int i = 0; i < 4; i++) {
		Move move;
		move.from = stringToSq(moves2[i]);
		move.to = stringToSq(moves2[i] + 2);
		move.piece = -1;
		for (int p = peon; p <= rey; p++) {
			if (BB_SQUARE(move.from) & t.piezas[side][p]) {
				move.piece = p;
				break;
			}
		}
		move.capture = -1;
		move.special = 0;
		move.promoPiece = 0;
		makeMove(&move, &t, side);
		side = !side;
	}
	uint64_t hash2 = t.hash;

	printf("hash1 = %lx, hash2 = %lx\n", hash1, hash2);
	if (hash1 == hash2)
		printf("Hashes match! makeMove works.\n");
	else
		printf("Hashes still differ – check other bugs.\n");
}
void testTT() {
	// Clear the table once before starting
	memset(tt, 0, sizeof(tt));

	Tablero t;
	initBoard(&t);
	struct timespec start, end;
	uint64_t nodes_before, nodes_after;

	// First search
	nodes = 0;
	clock_gettime(CLOCK_MONOTONIC, &start);
	moveScore best1 = negaMaxFixedDepth(&t, blancas, 5);
	clock_gettime(CLOCK_MONOTONIC, &end);
	double elapsed1 = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
	printf("First: nodes %lld time %.3f s nps %.0f\n", nodes, elapsed1, nodes / elapsed1);

	// Second search (same position, table now populated)
	nodes = 0;
	clock_gettime(CLOCK_MONOTONIC, &start);
	moveScore best2 = negaMaxFixedDepth(&t, blancas, 5);
	clock_gettime(CLOCK_MONOTONIC, &end);
	double elapsed2 = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
	printf("Second: nodes %lld time %.3f s nps %.0f\n", nodes, elapsed2, nodes / elapsed2);
}
void generateRookCaptures(moveLists * ml, color c, Tablero * t) {
	bitboard allRooks = t->piezas[c][torre];
	while (allRooks) {
		casilla from = __builtin_ctzll(allRooks);
		allRooks &= allRooks - 1;
		for (int i = 0; i < 4; i++) {
			bitboard ray = rookMask[from][i];
			bitboard blockers = ray & t->allOccupiedSquares;
			if (blockers) {
				int blockerSq;
				if (i < 2)
					blockerSq = __builtin_ctzll(blockers);
				else
					blockerSq = 63 - __builtin_clzll(blockers);
				// Only consider the blocker if it's an enemy piece (capture)
				if (BB_SQUARE(blockerSq) & t->allPieces[!c]) {
					int capture = -1;
					for (int p = peon; p <= rey; p++) {
						if (t->piezas[!c][p] & BB_SQUARE(blockerSq)) {
							capture = p;
							break;
						}
					}
					Move move = {from, blockerSq, torre, capture, 0, 0};
					ml->moves[ml->count++] = move;
				}
			}
		}
	}
}
void generateBishopCaptures(moveLists * ml, color c, Tablero * t) {
	bitboard allBishops = t->piezas[c][alfil];
	while (allBishops) {
		casilla from = __builtin_ctzll(allBishops);
		allBishops &= allBishops - 1;
		for (int i = 0; i < 4; i++) {
			bitboard ray = bishopMask[from][i];
			bitboard blockers = ray & t->allOccupiedSquares;
			if (blockers) {
				int blockerSq;
				if (i < 2)
					blockerSq = __builtin_ctzll(blockers);
				else
					blockerSq = 63 - __builtin_clzll(blockers);
				// Only consider the blocker if it's an enemy piece (capture)
				if (BB_SQUARE(blockerSq) & t->allPieces[!c]) {
					int capture = -1;
					for (int p = peon; p <= rey; p++) {
						if (t->piezas[!c][p] & BB_SQUARE(blockerSq)) {
							capture = p;
							break;
						}
					}
					Move move = {from, blockerSq, alfil, capture, 0, 0};
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
	bitboard allQueens = t->piezas[c][reina];
	while (allQueens) {
		casilla from = __builtin_ctzll(allQueens);
		allQueens &= allQueens - 1;
		// Rook directions
		for (int i = 0; i < 4; i++) {
			bitboard ray = rookMask[from][i];
			bitboard blockers = ray & t->allOccupiedSquares;
			if (blockers) {
				int blockerSq;
				if (i < 2)
					blockerSq = __builtin_ctzll(blockers);
				else
					blockerSq = 63 - __builtin_clzll(blockers);
				if (BB_SQUARE(blockerSq) & t->allPieces[!c]) {
					int capture = -1;
					for (int p = peon; p <= rey; p++) {
						if (t->piezas[!c][p] & BB_SQUARE(blockerSq)) {
							capture = p;
							break;
						}
					}
					Move move = {from, blockerSq, reina, capture, 0, 0};
					ml->moves[ml->count++] = move;
				}
			}
		}
		// Bishop directions
		for (int i = 0; i < 4; i++) {
			bitboard ray = bishopMask[from][i];
			bitboard blockers = ray & t->allOccupiedSquares;
			if (blockers) {
				int blockerSq;
				if (i > 1)
					blockerSq = __builtin_ctzll(blockers);
				else
					blockerSq = 63 - __builtin_clzll(blockers);
				if (BB_SQUARE(blockerSq) & t->allPieces[!c]) {
					int capture = -1;
					for (int p = peon; p <= rey; p++) {
						if (t->piezas[!c][p] & BB_SQUARE(blockerSq)) {
							capture = p;
							break;
						}
					}
					Move move = {from, blockerSq, reina, capture, 0, 0};
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
bool testUnmakeMove(Tablero * original, Move move, color c) {
	Tablero copy = *original;
	// Save old state for unmake (assuming unmakeMove requires these)
	casilla oldEp = original->enPassantSquare;
	uint8_t oldCastling = original->castlingRights;
	int oldHalf = original->halfmoveClock;
	int oldFull = original->fullMoves;

	makeMove(&move, &copy, c);
	unmakeMove(&move, &copy, c, oldEp, oldCastling, oldHalf, oldFull);

	// Compare copy with original
	if (memcmp(&copy, original, sizeof(Tablero)) == 0) {
		printf("unmakeMove passed for move %s\n", moveToStr(&move));
		return true;
	} else {
		printf("unmakeMove FAILED for move %s\n", moveToStr(&move));
		// Optional: print differences (e.g., compare allPieces, hash, etc.)
		return false;
	}
}
void testUnmakeAll() {
	Tablero original;
	Move move;

	// ------------------------------------------------------------
	// Test 1: Normal capture
	// Position: white pawn on e4, black pawn on d5
	initBoard(&original);
	// Clear all pieces first
	memset(&original, 0, sizeof(Tablero));
	// Place white pawn on e4 (square 28)
	original.piezas[blancas][peon] = BB_SQUARE(28);
	// Place black pawn on d5 (square 27)
	original.piezas[negras][peon] = BB_SQUARE(27);
	updateBoardCache(&original);
	original.hash = computeZobrist(&zobrist, &original, blancas);
	move = (Move){28, 27, peon, peon, 0, 0}; // capture, captured piece = pawn
	testUnmakeMove(&original, move, blancas);
	// Test 2: En passant capture
	// Position: white pawn on e5 (square 36), black pawn on d5 (square 35),
	//           en passant target d6 (square 43) is set (because black's last move was d7-d5)
	memset(&original, 0, sizeof(Tablero));
	original.piezas[blancas][peon] = BB_SQUARE(36); // e5
	original.piezas[negras][peon] = BB_SQUARE(35);	// d5
	original.enPassantSquare = 43;			// d6
	updateBoardCache(&original);
	original.hash = computeZobrist(&zobrist, &original, blancas);
	move = (Move){36, 43, peon, peon, 1, 0}; // from e5 to d6, capture pawn, special=1
	testUnmakeMove(&original, move, blancas);

	// ------------------------------------------------------------
	// Test 3: Castling (white kingside)
	// Position: white king e1 (4), white rook h1 (7), no pieces in between
	memset(&original, 0, sizeof(Tablero));
	original.piezas[blancas][rey] = BB_SQUARE(4);
	original.piezas[blancas][torre] = BB_SQUARE(7);
	original.castlingRights = WHITE_OO;
	updateBoardCache(&original);
	original.hash = computeZobrist(&zobrist, &original, blancas);
	move = (Move){4, 6, rey, -1, 2, 0}; // e1g1, special=2
	testUnmakeMove(&original, move, blancas);

	// ------------------------------------------------------------
	// Test 4: Promotion without capture
	// Position: white pawn on e7 (52), empty e8 (60)
	memset(&original, 0, sizeof(Tablero));
	original.piezas[blancas][peon] = BB_SQUARE(52);
	updateBoardCache(&original);
	original.hash = computeZobrist(&zobrist, &original, blancas);
	move = (Move){52, 60, peon, -1, 3, reina}; // promote to queen
	testUnmakeMove(&original, move, blancas);

	// ------------------------------------------------------------
	// Test 5: Promotion with capture
	// Position: white pawn on e7 (52), black rook on e8 (60)
	memset(&original, 0, sizeof(Tablero));
	original.piezas[blancas][peon] = BB_SQUARE(52);
	original.piezas[negras][torre] = BB_SQUARE(60);
	updateBoardCache(&original);
	original.hash = computeZobrist(&zobrist, &original, blancas);
	move = (Move){52, 60, peon, torre, 3, reina}; // capture and promote
	testUnmakeMove(&original, move, blancas);

	printf("All unmakeMove tests completed.\n");
}
static inline bitboard BB_FILE(int file) { return (C64(0x0101010101010101) << file); }
