#include <iostream>
#include <conio.h>

using namespace std;

const int WIDTH = 120, HEIGHT = 30;
const int INITIAL_HITPOINTS = 5;
const char PLATFORM = '=', BORDER = '#', EMPTY = ' ';

char** gameMatrix = nullptr;

int changePlayerPosition(int x, int y) {
    if (gameMatrix[x][y] == PLATFORM || gameMatrix[x][y] == BORDER)
        return 1;



    return 0;
}

void initializeGameMatrix(int width, int height) {
    gameMatrix = new char* [height];
    for (int i = 0; i < height; i++) {
        gameMatrix[i] = new char[width];
    }

    for (int j = 0; j < width; j++) {
        gameMatrix[0][j] = BORDER;
        gameMatrix[height - 1][j] = BORDER;
    }

    for (int i = 1; i < height - 1; i++) {
        gameMatrix[i][0] = BORDER;
        gameMatrix[i][width - 1] = BORDER;

        for (int j = 1; j < width - 1; j++) {
            gameMatrix[i][j] = EMPTY;
        }
    }
}

void visualizeArena(int width, int height) {
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            cout << gameMatrix[i][j];
        }
        cout << endl;
    }
}

int main()
{
    int hitpoints = INITIAL_HITPOINTS;
    int posX, posY;
    cout << "HP: " << hitpoints << endl;

    initializeGameMatrix(WIDTH, HEIGHT);
    visualizeArena(WIDTH, HEIGHT);

    /*while (true) {
        if (_kbhit() == true) {
            switch (_getch()) {
                case 'A':
                    changePlayerPosition(posX--, posY);
                    break;
                case 'D':
                    changePlayerPosition(posX++, posY);
                    break;
                case 'W':
                    changePlayerPosition(posX, posY--);
                    break;
            }
        }
    }*/
}
