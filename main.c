#include <raylib.h>

//constantes
const int screenWidth = 600;
const int screenHeight = 600;
const int blocks = 8;
const int blockSize = screenWidth / blocks;
//Enums/Structs
typedef enum pieceType{
	b_pawn, b_knight, b_bishop, b_rook, b_queen, b_king,
	w_pawn, w_knight, w_bishop, w_rook, w_queen, w_king,
	EMPTY,
	pieceCount
} PieceType;
typedef struct ChessPieces{
	Texture2D textures[pieceCount];
}ChessPieces;
//declaracion de funciones
void drawBoard();
void loadChessPieces(ChessPieces * pieces);
void unloadChessPieces(ChessPieces * pieces);
PieceType getPiece(char piece);
int main(){
	//init general
	InitWindow(screenWidth, screenHeight, "stockCat");
	SetTargetFPS(60);
	ChessPieces chessPieces;
	loadChessPieces(&chessPieces);
	
	while (!WindowShouldClose()){
		BeginDrawing();
        	ClearBackground(WHITE);
		drawBoard();

        	EndDrawing();
	}
	unloadChessPieces(&chessPieces);
	CloseWindow();
	return 0;
}
//funciones
void drawBoard(){
	for(int fila = 0; fila < blocks; fila++){
		for(int columna = 0; columna < blocks; columna++){
			DrawRectangle(
				columna * blockSize,
				fila * blockSize,
     				blockSize,
     				blockSize,
     				((fila + columna) % 2 == 0) ? WHITE : (Color) {118,150,86,255}
		 	);
		}
	}
}
void loadChessPieces(ChessPieces * pieces){
	pieces->textures[w_pawn] = LoadTexture("resources/8_bit/wp.png");
	pieces->textures[w_knight] = LoadTexture("resources/8_bit/wk.png");
	pieces->textures[w_bishop] = LoadTexture("resources/8_bit/wk.png");
	pieces->textures[w_rook] = LoadTexture("resources/8_bit/wr.png");
	pieces->textures[w_queen] = LoadTexture("resources/8_bit/wq.png");
	pieces->textures[w_king] = LoadTexture("resources/8_bit/wk.png");

	pieces->textures[b_pawn] = LoadTexture("resources/8_bit/wp.png");
	pieces->textures[b_knight] = LoadTexture("resources/8_bit/bk.png");
	pieces->textures[b_bishop] = LoadTexture("resources/8_bit/bk.png");
	pieces->textures[b_rook] = LoadTexture("resources/8_bit/br.png");
	pieces->textures[b_queen] = LoadTexture("resources/8_bit/bq.png");
	pieces->textures[b_king] = LoadTexture("resources/8_bit/bk.png");
}
void unloadChessPieces(ChessPieces * pieces){
	for(int i = 0; i < pieceCount; i++){
		UnloadTexture(pieces->textures[i]);
	}
}
PieceType getPiece(char piece){
	switch (piece) {
		case 'p' : return b_pawn;
		case 'n' : return b_knight;
		case 'b' : return b_bishop;
		case 'r' : return b_rook;
		case 'q' : return b_queen;
		case 'k' : return b_king;
		case 'P' : return w_pawn;
		case 'N' : return w_knight;
		case 'B' : return w_bishop;
		case 'R' : return w_rook;
		case 'Q' : return w_queen;
		case 'K' : return w_king;
		default : return EMPTY;

	}
}
