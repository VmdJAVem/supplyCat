#include "include/search.h"
#include "include/board.h"
#include "include/movegen.h"
#include "include/eval.h"

long long int TThits = 0, TTmisses = 0;
TTEntry tt[TT_SIZE];

bool isEqualMoves(Move * x, Move * y) {
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

void moveToMoveSort(moveLists * input, moveSort output[], int depth) {
	for (int i = 0; i < input->count; i++) {
		output[i] = scoreMoveForSorting(&input->moves[i], depth);
	}
}

int compareMoveSort(const void * a, const void * b) {
	const moveSort * ma = (const moveSort *)a;
	const moveSort * mb = (const moveSort *)b;
	return mb->sortingScore - ma->sortingScore;
}

void insertionSort(moveSort * moves, int count) {
	for (int i = 1; i < count; i++) {
		moveSort key = moves[i];
		int j = i - 1;
		while (j >= 0 && moves[j].sortingScore < key.sortingScore) {
			moves[j + 1] = moves[j];
			j--;
		}
		moves[j + 1] = key;
	}
}

float recursiveNegaMax(int depth, Tablero * t, color c, float alpha, float beta) {
	nodes++;
	int index = t->hash & TT_MASK;
	TTEntry * entry = &tt[index];
	if (entry->key == t->hash) {
		TThits++;
		if (entry->depth >= depth) {
			if (entry->flag == 0)
				return entry->score;
			if (entry->flag == 1 && entry->score >= beta)
				return beta;
			if (entry->flag == 2 && entry->score <= alpha)
				return alpha;
		}
	} else {
		TTmisses++;
	}
	if (stopRequested)
		return 0;
	if (t->halfmoveClock >= 100)
		return 0;
	if (isRepetition(t->hash))
		return 0;
	float oldAlpha = alpha;
	if (debug) {
		printf("DEBUG: recursiveNegaMax start, colorToMove = %d\n", c);
	}
	moveLists colorToMove = {0};
	generateLegalMoves(c, t, &colorToMove);
	if (colorToMove.count == 0) {
		casilla kingSq = __builtin_ctzll(t->piezas[c][rey]);
		if (isAttacked(t, kingSq, !c)) {
			return -MATE_SCORE;
		} else {
			return 0;
		}
	}
	if (depth == 0) {
		return quiescence(t, c, alpha, beta, 4);
	}

	casilla kingSq = __builtin_ctzll(t->piezas[c][rey]);
	bool inCheck = isAttacked(t, kingSq, !c);

	if (!inCheck && depth >= 3) {
		int r = 2;
		int nullDepth = depth - 1 - r;
		if (nullDepth > 0) {
			int oldEp = t->enPassantSquare;
			t->enPassantSquare = -1;
			t->hash ^= zobrist.side;
			float nullScore = -recursiveNegaMax(nullDepth, t, !c, -beta, -alpha);
			t->hash ^= zobrist.side;
			t->enPassantSquare = oldEp;
			if (nullScore >= beta) {
				return beta;
			}
		}
	}

	if (nodes % 1024 == 0) {
		if (inputAvaliable()) {
			char buffer[4096];
			if (fgets(buffer, sizeof(buffer), stdin)) {
				proccesUCICommands(buffer, t);
			}
		}
	}
	moveSort moves[256];
	moveToMoveSort(&colorToMove, moves, depth);
	insertionSort(moves, colorToMove.count);
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
		positionHashes[positionHashCount++] = t->hash;
		float score = -recursiveNegaMax(depth - 1, t, !c, -beta, -alpha);
		positionHashCount--;
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
		flag = 2;
	else if (alpha >= beta)
		flag = 1;
	else
		flag = 0;

	if (entry->key != t->hash || entry->age != searchAge || entry->depth <= depth) {
		entry->key = t->hash;
		entry->depth = depth;
		entry->score = alpha;
		entry->flag = flag;
		entry->age = searchAge;
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
	searchAge++;
	if (colorToMove.count == 0) {
		casilla kingSq = __builtin_ctzll(t->piezas[c][rey]);
		float score;
		if (isAttacked(t, kingSq, !c)) {
			score = -MATE_SCORE;
		} else {
			score = 0;
		}
		moveScore output = {
		    .move = {0},
		    .score = score,
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
					char buffer[4096];
					if (fgets(buffer, sizeof(buffer), stdin)) {
						proccesUCICommands(buffer, t);
					}
				}
			}
			if (stopRequested)
				break;
			casilla oldEp = t->enPassantSquare;
			uint8_t oldCastling = t->castlingRights;
			int oldHalf = t->halfmoveClock;
			int oldFull = t->fullMoves;
			makeMove(&colorToMove.moves[i], t, c);
			positionHashes[positionHashCount++] = t->hash;
			float score = -recursiveNegaMax(depth, t, !c, -beta, -alpha);
			positionHashCount--;
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
		struct timespec now;
		clock_gettime(CLOCK_MONOTONIC, &now);
		long long elapsed = (now.tv_sec - start.tv_sec) * 1000LL + (now.tv_nsec - start.tv_nsec) / 1000000LL;
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
	searchAge++;
	moveLists colorToMove = {0};
	generateAllMoves(c, t, &colorToMove);
	memset(history, 0, sizeof(history));
	if (colorToMove.count == 0) {
		casilla kingSq = __builtin_ctzll(t->piezas[c][rey]);
		float score;
		if (isAttacked(t, kingSq, !c)) {
			score = -MATE_SCORE;
		} else {
			score = 0;
		}
		moveScore output = {.move = {0}, .score = score};
		return output;
	}

	float bestScore = -INFINITY;
	Move bestMove = {0};
	float alpha = -INFINITY;
	float beta = INFINITY;

	for (int i = 0; i < colorToMove.count; i++) {
		if (nodes % 1024 == 0) {
			if (inputAvaliable()) {
				char buffer[4096];
				if (fgets(buffer, sizeof(buffer), stdin)) {
					proccesUCICommands(buffer, t);
				}
			}
		}
		if (stopRequested)
			break;
		casilla oldEp = t->enPassantSquare;
		uint8_t oldCastling = t->castlingRights;
		int oldHalf = t->halfmoveClock;
		int oldFull = t->fullMoves;
		makeMove(&colorToMove.moves[i], t, c);
		positionHashes[positionHashCount++] = t->hash;
		float score = -recursiveNegaMax(depth - 1, t, !c, -beta, -alpha);
		positionHashCount--;
		unmakeMove(&colorToMove.moves[i], t, c, oldEp, oldCastling, oldHalf, oldFull);

		if (score > bestScore) {
			bestScore = score;
			bestMove = colorToMove.moves[i];
			alpha = score;
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

float quiescence(Tablero * t, color c, float alpha, float beta, int qdepth) {
	if (stopRequested)
		return 0;
	moveLists captures = {0};
	generateCaptures(c, t, &captures);
	if (qdepth <= 0) {
		return boardEval(t, c, captures.count, -1);
	}
	float standPat = boardEval(t, c, captures.count, -1);
	if (standPat >= beta) {
		return beta;
	}
	if (standPat > alpha) {
		alpha = standPat;
	}
	moveSort moves[256];
	moveToMoveSort(&captures, moves, 0);
	insertionSort(moves, captures.count);

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

void testZobrist() {
	Tablero t;
	initBoard(&t);
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
	memset(tt, 0, sizeof(tt));
	TThits = 0;
	TTmisses = 0;

	Tablero t;
	initBoard(&t);
	struct timespec start, end;
	nodes = 0;
	clock_gettime(CLOCK_MONOTONIC, &start);
	negaMaxFixedDepth(&t, blancas, 5);
	clock_gettime(CLOCK_MONOTONIC, &end);
	double elapsed1 = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
	long long ttProbes1 = TThits + TTmisses;
	printf("First: nodes %lld time %.3f s nps %.0f TT probes %lld hit rate %.2f%%\n", nodes, elapsed1,
	       nodes / elapsed1, ttProbes1, ttProbes1 ? 100.0 * TThits / ttProbes1 : 0);

	nodes = 0;
	TThits = 0;
	TTmisses = 0;
	clock_gettime(CLOCK_MONOTONIC, &start);
	negaMaxFixedDepth(&t, blancas, 5);
	clock_gettime(CLOCK_MONOTONIC, &end);
	double elapsed2 = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
	long long ttProbes2 = TThits + TTmisses;
	printf("Second: nodes %lld time %.3f s nps %.0f TT probes %lld hit rate %.2f%%\n", nodes, elapsed2,
	       nodes / elapsed2, ttProbes2, ttProbes2 ? 100.0 * TThits / ttProbes2 : 0);
}

void testNPS() {
	struct timespec start, end;
	double totalTime = 0;
	long long totalNodes = 0;

	printf("Running NPS benchmark...\n\n");

	for (int depth = 1; depth <= 10; depth++) {
		Tablero t;
		initBoard(&t);
		memset(tt, 0, sizeof(tt));
		TThits = 0;
		TTmisses = 0;

		nodes = 0;
		clock_gettime(CLOCK_MONOTONIC, &start);
		negaMaxFixedDepth(&t, blancas, depth);
		clock_gettime(CLOCK_MONOTONIC, &end);

		double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
		totalTime += elapsed;
		totalNodes += nodes;

		long long ttProbes = TThits + TTmisses;
		printf("depth %d: %lld nodes %.3f s nps %.0f TT hit rate %.1f%%\n", depth, nodes, elapsed,
		       elapsed > 0 ? nodes / elapsed : 0, ttProbes ? 100.0 * TThits / ttProbes : 0);
	}

	printf("\nTotal: %lld nodes %.3f s nps %.0f\n", totalNodes, totalTime,
	       totalTime > 0 ? totalNodes / totalTime : 0);
}

bool testUnmakeMove(Tablero * original, Move move, color c) {
	Tablero copy = *original;
	casilla oldEp = original->enPassantSquare;
	uint8_t oldCastling = original->castlingRights;
	int oldHalf = original->halfmoveClock;
	int oldFull = original->fullMoves;

	makeMove(&move, &copy, c);
	unmakeMove(&move, &copy, c, oldEp, oldCastling, oldHalf, oldFull);

	if (memcmp(&copy, original, sizeof(Tablero)) == 0) {
		printf("unmakeMove passed for move %s\n", moveToStr(&move));
		return true;
	} else {
		printf("unmakeMove FAILED for move %s\n", moveToStr(&move));
		return false;
	}
}

void testUnmakeAll() {
	Tablero original;
	Move move;

	memset(&original, 0, sizeof(Tablero));
	original.piezas[blancas][peon] = BB_SQUARE(28);
	original.piezas[negras][peon] = BB_SQUARE(27);
	updateBoardCache(&original);
	original.hash = computeZobrist(&zobrist, &original, blancas);
	move = (Move){28, 27, peon, peon, 0, 0};
	testUnmakeMove(&original, move, blancas);

	memset(&original, 0, sizeof(Tablero));
	original.piezas[blancas][peon] = BB_SQUARE(36);
	original.piezas[negras][peon] = BB_SQUARE(35);
	original.enPassantSquare = 43;
	updateBoardCache(&original);
	original.hash = computeZobrist(&zobrist, &original, blancas);
	move = (Move){36, 43, peon, peon, 1, 0};
	testUnmakeMove(&original, move, blancas);

	memset(&original, 0, sizeof(Tablero));
	original.piezas[blancas][rey] = BB_SQUARE(4);
	original.piezas[blancas][torre] = BB_SQUARE(7);
	original.castlingRights = WHITE_OO;
	updateBoardCache(&original);
	original.hash = computeZobrist(&zobrist, &original, blancas);
	move = (Move){4, 6, rey, -1, 2, 0};
	testUnmakeMove(&original, move, blancas);

	memset(&original, 0, sizeof(Tablero));
	original.piezas[blancas][peon] = BB_SQUARE(52);
	updateBoardCache(&original);
	original.hash = computeZobrist(&zobrist, &original, blancas);
	move = (Move){52, 60, peon, -1, 3, reina};
	testUnmakeMove(&original, move, blancas);

	memset(&original, 0, sizeof(Tablero));
	original.piezas[blancas][peon] = BB_SQUARE(52);
	original.piezas[negras][torre] = BB_SQUARE(60);
	updateBoardCache(&original);
	original.hash = computeZobrist(&zobrist, &original, blancas);
	move = (Move){52, 60, peon, torre, 3, reina};
	testUnmakeMove(&original, move, blancas);

	printf("All unmakeMove tests completed.\n");
}