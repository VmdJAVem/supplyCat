#include "include/uci.h"
#include "include/board.h"
#include "include/search.h"

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
		positionHashCount = 0;
		return;
	}

	char * firstCommand = strtok(command, " \t\n\r\f\v");
	if (!firstCommand)
		return;

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
			positionHashCount = 0;
			positionHashes[positionHashCount++] = t->hash;
			char * movesToken = strtok(NULL, " ");
			if (movesToken && strcmp(movesToken, "moves") == 0) {
				char * move = strtok(NULL, " ");
				while (move) {
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
					positionHashes[positionHashCount++] = t->hash;
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
			positionHashCount = 0;
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
					positionHashes[positionHashCount++] = t->hash;
					move = strtok(NULL, " ");
					colorToMove = !colorToMove;
				}
			}
			t->hash = computeZobrist(&zobrist, t, colorToMove);
			return;
		}
	}
	if (strcmp(firstCommand, "go") == 0) {
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
					if (i == 7) {
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
			nodes = 0;
			struct timespec start, end;
			clock_gettime(CLOCK_MONOTONIC, &start);
			bestMove = negaMaxFixedDepth(t, colorToMove, parameters.depth);
			clock_gettime(CLOCK_MONOTONIC, &end);
			double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
			double nps = nodes / elapsed;
		} else if (parameters.movetime != -1) {
			bestMove = negaMax(t, colorToMove, parameters.movetime);
		} else if (parameters.infinite) {
			bestMove = negaMax(t, colorToMove, -1);
		} else if (parameters.wtime != -1 && parameters.btime != -1) {
			int clockTime = (colorToMove == blancas) ? parameters.wtime : parameters.btime;
			int inc = (colorToMove == blancas) ? parameters.winc : parameters.binc;
			int movesLeft = (parameters.movestogo > 0) ? parameters.movestogo : 40;
			int base = clockTime / (movesLeft + 5);
			int timeForMove = base + inc / 2;

			if (timeForMove > clockTime / 2)
				timeForMove = clockTime / 2;

			if (timeForMove < 20)
				timeForMove = 20;
			bestMove = negaMax(t, colorToMove, timeForMove);
		} else {
			bestMove = negaMax(t, colorToMove, -1);
		}
		if (debug) {
			printf("DEBUG: from=%d to=%d special=%d promo=%d\n", bestMove.move.from, bestMove.move.to,
			       bestMove.move.special, bestMove.move.promoPiece);
		}
		printf("bestmove %s\n", moveToStr(&bestMove.move));
		fflush(stdout);
		return;
	}
}