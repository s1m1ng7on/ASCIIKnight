#include <iostream>
#include <conio.h>
#include <cstdlib>
#include <ctime>
#include <windows.h>

using namespace std;

const int WIDTH = 90, HEIGHT = 25;
const int INITIAL_HITPOINTS = 5;
const char PLATFORM = '=', BORDER = '#', EMPTY = ' ', PLAYER = '@';
const int JUMP_STRENGTH = 2;
const int GRAVITY = 1;

int hitpoints = INITIAL_HITPOINTS;
int playerX = (WIDTH / 2) - 1;
int playerY = (HEIGHT / 2) - 1;

int jumpsLeft = 2;
int velY = 0;

char** gameMatrix = nullptr;

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

HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

void moveCursor(int x, int y) {
    COORD c;
    c.X = (SHORT)x;
    c.Y = (SHORT)y;
    SetConsoleCursorPosition(hConsole, c);
}

void hideCursor() {
    CONSOLE_CURSOR_INFO info;
    GetConsoleCursorInfo(hConsole, &info);
    info.bVisible = FALSE;
    SetConsoleCursorInfo(hConsole, &info);
}

int randInt(int a, int b) {
    return a + rand() % (b - a + 1);
}

void generatePlatforms() {
    for (int i = 0; i < 3; i++) {
        int randX = randInt(1, WIDTH - 2);
        int randY = randInt(1, HEIGHT - 2);
        int length = randInt(1, WIDTH - 3);
        
        int j = 0;
        while (j < length && gameMatrix[randY][j + randX] != BORDER && gameMatrix[randY][j + randX] != PLATFORM) {
            gameMatrix[randY][j + randX] = PLATFORM;
            j++;
        }
    }
}

bool isBlocked(int posX, int posY) {
    return gameMatrix[posY][posX] == BORDER || gameMatrix[posY][posX] == PLATFORM;
}

void moveHorizontal(int dx) {
    int newX = playerX + dx;

    if (newX < 1 || newX > WIDTH - 2)
        return;

    if (!isBlocked(newX, playerY))
        playerX = newX;
}

void jump() {
    if (jumpsLeft <= 0) return;

    velY = -JUMP_STRENGTH;
    jumpsLeft--;
}

void handleInput() {
    if (_kbhit() == true) {
        switch (_getch()) {
            case 'A':
            case 'a':
                moveHorizontal(-1);
                break;
            case 'D':
            case 'd':
                moveHorizontal(1);
                break;
            case 'W':
            case 'w':
                jump();
                break;
        }
    }
}

void render() {
    moveCursor(0, 0);

    cout << "HP: " << hitpoints << endl;

    for (int i = 0; i < HEIGHT; i++) {
        for (int j = 0; j < WIDTH; j++) {
            cout << (!(i == playerY && j == playerX)
                ? gameMatrix[i][j]
                : PLAYER);
        }
        cout << endl;
    }
}

void gravityCheck() {
    bool onGround = isBlocked(playerX, playerY + 1);

    if (onGround && velY >= 0) {
        velY = 0;
        jumpsLeft = 2;
    }
    else {
        velY += GRAVITY;
    }

    int steps = abs(velY);
    int dir = (velY < 0) ? -1 : 1;

    for (int s = 0; s < steps; s++) {
        int newY = playerY + dir;

        if (newY < 1 || newY > HEIGHT - 2) {
            velY = 0;
            break;
        }

        if (gameMatrix[newY][playerX] == BORDER || gameMatrix[newY][playerX] == PLATFORM) {
            velY = 0;
            break;
        }

        playerY = newY;
    }
}

int main()
{
    srand((unsigned)time(nullptr));

    hideCursor();

    initializeGameMatrix(WIDTH, HEIGHT);
    generatePlatforms();

    while (true) {
        handleInput();
        gravityCheck();
        render();

        Sleep(40);
    }
}
