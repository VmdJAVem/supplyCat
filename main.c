#include <raylib.h>
#include <stdbool.h>

// constantes
const int screenWitdh = 750; // espacio vacio 150 * 650
const int screenHeight = 650;
const int boardWitdh = 600;
const int boardHeight = 600;
const int blocks = 8;
const int blockSize = boardWitdh / blocks;
const char * initBoard = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR";
// Enums/Structs
typedef enum pieceType{
	b_pawn, b_knight, b_bishop, b_rook, b_queen, b_king,
	w_pawn, w_knight, w_bishop, w_rook, w_queen, w_king,
	EMPTY,
	pieceCount
} PieceType;
typedef struct ChessPieces{
	Texture2D textures[pieceCount];
}ChessPieces;
// declaracion de funciones
void drawBoard();
void loadChessPieces(ChessPieces * pieces);
void unloadChessPieces(ChessPieces * pieces);
PieceType getPiece(char piece);
void drawPiece(Texture2D texture, int columna, int fila);
void parseBoard(const char * board, ChessPieces * pieces);
void drawBtn(Texture2D btn, int x, int y);
void drawbg(Texture2D bg);
int main(){
	// init general
	InitWindow(screenWitdh, screenHeight, "stockCat");
	SetTargetFPS(60);
	ChessPieces chessPieces;
	loadChessPieces(&chessPieces);
	Texture2D quitBtn = LoadTexture("resources/sprites/quit.png");
	Texture2D empateBtn = LoadTexture("resources/sprites/draw.png");
	Texture2D background =  LoadTexture("resources/sprites/bg.png");

	// inicializar el Vector2 que contiene la posicion del mouse
	Vector2 mousePos = {0.0f, 0.0f};

	while(!WindowShouldClose()){
        	ClearBackground(BLACK);
		drawbg(background);
		drawBoard();
		DrawText("White's turn", 615 , 10,20, RAYWHITE);
		DrawText("Score: 0-0",615, 40, 20, RAYWHITE);
		DrawText("Playing",240, 615,25,RAYWHITE);
		drawBtn(quitBtn,600,75);
		drawBtn(empateBtn,600,150);
		parseBoard(initBoard, &chessPieces);
        	EndDrawing();
	}
	unloadChessPieces(&chessPieces);
	CloseWindow();
	return 0;
}
// funciones
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
	pieces->textures[w_pawn] = LoadTexture("resources/classic/wp.png");
	pieces->textures[w_knight] = LoadTexture("resources/classic/wn.png");
	pieces->textures[w_bishop] = LoadTexture("resources/classic/wb.png");
	pieces->textures[w_rook] = LoadTexture("resources/classic/wr.png");
	pieces->textures[w_queen] = LoadTexture("resources/classic/wq.png");
	pieces->textures[w_king] = LoadTexture("resources/classic/wk.png");

	pieces->textures[b_pawn] = LoadTexture("resources/classic/bp.png");
	pieces->textures[b_knight] = LoadTexture("resources/classic/bn.png");
	pieces->textures[b_bishop] = LoadTexture("resources/classic/bb.png");
	pieces->textures[b_rook] = LoadTexture("resources/classic/br.png");
	pieces->textures[b_queen] = LoadTexture("resources/classic/bq.png");
	pieces->textures[b_king] = LoadTexture("resources/classic/bk.png");

	for(int i = 0; i < pieceCount; i++){
		if(pieces->textures[i].id == 0){
			TraceLog(LOG_WARNING, "Failed to load textures  for pieces %d", i);
		}
	}
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
void drawPiece(Texture2D texture, int columna, int fila){
	DrawTexturePro(
		texture,
		(Rectangle){0,0,(float)texture.width,(float) texture.height}, // Full Rectangle
		(Rectangle){
			columna * blockSize,
			fila * blockSize,
			blockSize,
			blockSize
		},
		(Vector2){0,0}, // origin offset (no se necesita)
		0.0f, // rotation
		WHITE
	);
}
void parseBoard(const char * board, ChessPieces * pieces){
	int fila = 0; int columna = 0;
	for(int i = 0;board[i] != '\0';i++){
		char c = board[i];

		if(c == '/'){
			fila++;
			columna = 0;
		}
		else if(c >= '1' && c <= '8'){
			columna += (c - '0');
		}
		else{
			PieceType p = getPiece(c);
			if(p != EMPTY){
				drawPiece(pieces->textures[p], columna,fila);
				columna++;
			}
		}
	}
}
void drawBtn(Texture2D btn, int x, int y){
	DrawTexture(btn,x,y, RAYWHITE);

}
void drawbg(Texture2D bg){
	DrawTexture(bg,0,0,RAYWHITE);
}
