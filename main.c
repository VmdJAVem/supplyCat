#include <raylib.h>
//macros
const int screenWidth = 800;
const int screenHeight = 800;
const int blocks = 8;
const int blockSize = screenWidth / blocks;
//funciones
void drawBoard(){
	for(int fila = 0; fila < blocks; fila++){
		for(int columna = 0; columna < blocks; columna++){
			DrawRectangle(
				columna * blockSize,
				fila * blockSize,
     				blockSize,
     				blockSize,
     				((fila + columna) % 2 == 0) ? WHITE : PINK
		 	);
		}
	}
}

int main(void){
	//init general
	InitWindow(screenWidth, screenHeight, "stockCat");
	SetTargetFPS(60);
	
	while (!WindowShouldClose()){
		BeginDrawing();
        	ClearBackground(WHITE);
		drawBoard();

        	EndDrawing();
	}
	CloseWindow();
	return 0;
}
