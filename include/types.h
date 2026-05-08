#ifndef TYPES_H
#define TYPES_H

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

#define C64(constantU64) constantU64##ULL
#define BB_SQUARE(sq) (1ULL << (sq))
#define RANK(r) (C64(0xFF) << (8 * (r)))
#define C8 ((uint8_t)123)
#define MAX_DEPTH 128
#define MATE_SCORE 100000

#define TT_SIZE (1 << 23)
#define TT_MASK (TT_SIZE - 1)

#define MAX_GAME_LENGTH 1024

typedef enum { blancas, negras } color;
typedef enum { peon, caballo, alfil, torre, reina, rey } tipoDePieza;

extern int sortingValues[6];

typedef enum {
	a1, b1, c1, d1, e1, f1, g1, h1,
	a2, b2, c2, d2, e2, f2, g2, h2,
	a3, b3, c3, d3, e3, f3, g3, h3,
	a4, b4, c4, d4, e4, f4, g4, h4,
	a5, b5, c5, d5, e5, f5, g5, h5,
	a6, b6, c6, d6, e6, f6, g6, h6,
	a7, b7, c7, d7, e7, f7, g7, h7,
	a8, b8, c8, d8, e8, f8, g8, h8
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

extern const uint8_t WHITE_OO;
extern const uint8_t WHITE_OOO;
extern const uint8_t BLACK_OO;
extern const uint8_t BLACK_OOO;

typedef struct {
	casilla from;
	casilla to;
	tipoDePieza piece;
	int capture;
	int special;
	int promoPiece;
} Move;

typedef struct {
	uint64_t key;
	int depth;
	float score;
	uint8_t flag;
	uint8_t age;
} TTEntry;

extern TTEntry tt[TT_SIZE];

typedef struct {
	Move moves[256];
	int count;
} moveLists;

typedef struct {
	Move move;
	float score;
} moveScore;

typedef struct {
	int sortingScore;
	Move move;
} moveSort;

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
	uint64_t pieces[2][6][64];
	uint64_t side;
	uint64_t castling[4];
	uint64_t enPassant[8];
} Zobrist;

extern bitboard knightAttacks[64];
extern bitboard kingAttacks[64];
extern bitboard pawnAttacks[2][64];
extern bitboard rookMask[64][4];
extern bitboard bishopMask[64][4];

extern Tablero tablero;
extern volatile bool stopRequested;
extern bool isPlaying;
extern color colorToMove;
extern bool debug;
extern Move killerMoves[MAX_DEPTH][2];
extern int history[64][64];
extern uint64_t positionHashes[MAX_GAME_LENGTH];
extern int positionHashCount;
extern Zobrist zobrist;
extern goParameters parameters;
extern long long nodes;
extern int searchAge;
extern int positionalValues[2][6][64];

extern long long int TThits;
extern long long int TTmisses;

#endif