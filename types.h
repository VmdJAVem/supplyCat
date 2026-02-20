#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>
#include <stdbool.h>

// Basic types
typedef uint64_t bitboard;

// Macros
#define C64(constantU64) constantU64##ULL
#define BB_SQUARE(sq) (1ULL << (sq))
#define RANK(r) (C64(0xFF) << (8 * (r)))

// Enums
typedef enum {
	blancas, negras
} color;

typedef enum {
	peon, caballo, alfil, torre, reina, rey
} tipoDePieza;

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

// Castling rights constants
extern const uint8_t WHITE_OO;
extern const uint8_t WHITE_OOO;
extern const uint8_t BLACK_OO;
extern const uint8_t BLACK_OOO;

// Board structure
typedef struct {
	bitboard piezas[2][6];
	bitboard allPieces[2];
	bitboard allOccupiedSquares;
	uint8_t castlingRights;
	casilla enPassantSquare;
	int halfmoveClock;
	int fullMoves;
} Tablero;

// Move structure
typedef struct {
	casilla from;
	casilla to;
	tipoDePieza piece;
	int capture;
	int special; // 0 normal, 1 en passant, 2 castling, 3 promo
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

// Go parameters
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

// Global variables (declared as extern)
extern bool debug;
extern bool stopRequested;
extern color colorToMove;
extern long long nodes;
extern goParameters parameters;
extern bool isPlaying;

#endif // TYPES_H
