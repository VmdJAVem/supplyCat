#define _POSIX_C_SOURCE 199309L
#include "uci.h"
#include "board.h"
#include "search.h"
#include "move.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/select.h>
#include <unistd.h>

bool inputAvaliable(void) {
	struct timeval tv = {0, 0};
	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(0, &fds);
	int ret = select(1, &fds, NULL, NULL, &tv);
	return (ret > 0);
}

void proccesUCICommands(char command[256], Tablero *t) {
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
	} else if (strcmp(command, "debug\n") == 0) {
		debug = !debug;
		return;
	}

	if (debug) {
		printf("DEBUG: full command = '%s'\n", command);
		fflush(stdout);
	}

	char *firstCommand = strtok(command, " \t\n\r\f\v");

	if (debug) {
		printf("DEBUG: firstCommand = '%s'\n", firstCommand);
		fflush(stdout);
	}

	if (strcmp(firstCommand, "position") == 0) {
		printBitboard(t->allOccupiedSquares);
		char *secondCommand = strtok(NULL, " \t\n\r\f\v");

		if (debug) {
			printf("DEBUG: secondCommand = '%s'\n", secondCommand);
		}

		if (strcmp(secondCommand, "startpos") == 0) {
			initBoard(t);

			if (debug) {
				printf("DEBUG: after initBoard, white pawns = %lx\n", t->piezas[blancas][peon]);
			}

			colorToMove = blancas;
			char *movesToken = strtok(NULL, " ");

			if (movesToken != NULL && strcmp(movesToken, "moves") == 0) {
				char *move = strtok(NULL, " ");
				while (move != NULL) {
					casilla to = stringToSq(move + 2);
					casilla from = stringToSq(move);
					tipoDePieza promo = 0;
					tipoDePieza piece;
					int special = 0;
					tipoDePieza capture = 0;

					for (int i = peon; i <= rey; i++) {
						if (BB_SQUARE(from) & t->piezas[colorToMove][i]) {
							piece = i;
							break;
						}
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
			printBitboard(t->allOccupiedSquares);
		} else if (strcmp(secondCommand, "fen") == 0) {
			char *fenString = strtok(NULL, " ");
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
					continue;
				} else {
					color c = isupper(currentChar) ? blancas : negras;
					tipoDePieza piece = charToPiece(currentChar);
					casilla square = rank * 8 + file;
					t->piezas[c][piece] |= BB_SQUARE(square);
					file++;
				}
			}

			updateBoardCache(t);
			char *activeColor = strtok(NULL, " ");
			colorToMove = strcmp(activeColor, "w") ? negras : blancas;

			char *castlingRights = strtok(NULL, " ");
			if (strchr(castlingRights, '-') != NULL) {
				t->castlingRights = 0;
			} else {
				if (strchr(castlingRights, 'K') != NULL) t->castlingRights |= WHITE_OO;
				if (strchr(castlingRights, 'Q') != NULL) t->castlingRights |= WHITE_OOO;
				if (strchr(castlingRights, 'k') != NULL) t->castlingRights |= BLACK_OO;
				if (strchr(castlingRights, 'q') != NULL) t->castlingRights |= BLACK_OOO;
			}

			char *enPassantSquare = strtok(NULL, " ");
			if (strchr(enPassantSquare, '-')) {
				t->enPassantSquare = -1;
			} else {
				t->enPassantSquare = stringToSq(enPassantSquare);
			}

			char *halfmoveClock = strtok(NULL, " ");
			t->halfmoveClock = atoi(halfmoveClock);
			char *fullMove = strtok(NULL, " ");
			t->fullMoves = atoi(fullMove);

			char *movesToken = strtok(NULL, " ");
			if (movesToken != NULL && strcmp(movesToken, "moves") == 0) {
				char *move = strtok(NULL, " ");
				while (move != NULL) {
					casilla to = stringToSq(move + 2);
					casilla from = stringToSq(move);
					tipoDePieza promo = 0;
					tipoDePieza piece;
					int special = 0;
					tipoDePieza capture = 0;

					for(int i = peon; i <= rey; i++){
						if(BB_SQUARE(from) & t->piezas[colorToMove][i]){
							piece = i;
							break;
						}
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

					if(piece == peon && to == t->enPassantSquare){
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

		char *secondCommand = strtok(NULL, " \t\n\r\f\v");
		char possible2ndCommands[8][10] = {"wtime", "btime", "winc", "binc", "movestogo", "depth", "movetime", "infinite"};

		if (secondCommand == NULL) {
			if (debug) {
				printf("DEBUG: before negaMax, white pawns = %lx\n", t->piezas[blancas][peon]);
			}
			moveScore bestMove = negaMax(t, colorToMove, -1);
			printf("bestmove %s\n", moveToStr(&bestMove.move));
		} else {
			while (secondCommand != NULL) {
				for (int i = 0; i <= 7; i++) {
					if (strcmp(possible2ndCommands[i], secondCommand) == 0) {
						char *valueStr = strtok(NULL, " \t\n\r\f\v");
						switch (i) {
							case 0: parameters.wtime = atoi(valueStr); break;
							case 1: parameters.btime = atoi(valueStr); break;
							case 2: parameters.winc = atoi(valueStr); break;
							case 3: parameters.binc = atoi(valueStr); break;
							case 4: parameters.movestogo = atoi(valueStr); break;
							case 5: parameters.depth = atoi(valueStr); break;
							case 6: parameters.movetime = atoi(valueStr); break;
							case 7: parameters.infinite = true; break;
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
					if (parameters.winc != -1) increment = parameters.winc;
				} else if (colorToMove == negras) {
					clockTime = parameters.btime;
					if (parameters.binc != -1) increment = parameters.binc;
				}

				int timeForThisMove = clockTime / (parameters.movestogo == -1 ? 40 : parameters.movestogo) + increment;
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
