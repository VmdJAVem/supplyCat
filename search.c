#define _POSIX_C_SOURCE 199309L
#include "search.h"
#include "board.h"
#include "movegen.h"
#include "uci.h"
#include <stdio.h>
#include <math.h>
#include <time.h>
#include "move.h"

// Global variables definition
bool debug = false;
bool stopRequested = false;
color colorToMove = blancas;
long long nodes = 0;
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
bool isPlaying = true;

float recursiveNegaMax(int depth, Tablero *t, color c, float alpha, float beta) {
	nodes++;

	if (debug) {
		printf("DEBUG: recursiveNegaMax start, colorToMove = %d\n", c);
	}

	moveLists colorToMove = {0};
	generateAllMoves(c, t, &colorToMove);

	if (depth == 0 || colorToMove.count == 0) {
		return boardEval(t, c);
	}

	if (inputAvaliable()) {
		char buffer[256];
		if (fgets(buffer, sizeof(buffer), stdin)) {
			proccesUCICommands(buffer, t);
		}
	}

	for (int i = 0; i < colorToMove.count; i++) {
		Tablero temp = *t;
		makeMove(&colorToMove.moves[i], &temp, c);
		float score = -recursiveNegaMax(depth - 1, &temp, !c, -beta, -alpha);

		if (score > alpha) {
			alpha = score;
		}
		if (alpha >= beta) {
			break;
		}
	}
	return alpha;
}

moveScore negaMax(Tablero *t, color c, int timeLimit) {
	if (debug) {
		printf("DEBUG: negaMax start, colorToMove = %d\n", c);
		fflush(stdout);
	}

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
			if (inputAvaliable()) {
				char buffer[256];
				if (fgets(buffer, sizeof(buffer), stdin)) {
					proccesUCICommands(buffer, t);
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
				long long elapsed = (now.tv_sec - start.tv_sec) * 1000LL + 
					(now.tv_nsec - start.tv_nsec) / 1000000LL;
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

	moveScore bm = {
		.move = bestMove,
		.score = bestScore
	};
	printf("%lld nodes searched\n", nodes);
	return bm;
}

moveScore negaMaxFixedDepth(Tablero *t, color c, int depth) {
	moveLists colorToMove = {0};
	generateAllMoves(c, t, &colorToMove);

	if (colorToMove.count == 0) {
		moveScore output = {.move = {0}, .score = boardEval(t, c)};
		return output;
	}

	float bestScore = -INFINITY;
	Move bestMove = {0};
	float alpha = -INFINITY;
	float beta = INFINITY;

	for (int i = 0; i < colorToMove.count; i++) {
		Tablero temp = *t;
		makeMove(&colorToMove.moves[i], &temp, c);
		float score = -recursiveNegaMax(depth - 1, &temp, !c, -beta, -alpha);

		if (score > bestScore) {
			bestScore = score;
			bestMove = colorToMove.moves[i];
			alpha = score;
		}
	}

	moveScore result = {.move = bestMove, .score = bestScore};
	printf("%lld nodes searched\n", nodes);
	return result;
}
