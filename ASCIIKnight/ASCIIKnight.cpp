#include <iostream>
#include <conio.h>
#include <cstdlib>
#include <ctime>
#include <windows.h>

using namespace std;

struct Enemy {
    int x, y;
    char type;
    int hp;
    int attackCooldown;
    bool alive;
};

enum Direction {
    Up,
    Left,
    Down,
    Right
};

const int WIDTH = 90, HEIGHT = 25;
const int INITIAL_HITPOINTS = 5;
const int WAVES_NUMBER = 5;

const char PLAYER = '@',
           BASIC_WALKER = 'E',
           JUMPER = 'J',
           FLIER = 'F',
           CRAWLER = 'C',
           BOSS = 'B';

const char EMPTY = ' ',
           PLATFORM = '=',
           BORDER = '#';

const int JUMP_STRENGTH = 2;
const int GRAVITY = 1;
const int ATTACK_CELLS = 3;
const int ENEMY_ATTACK_COOLDOWN = 20;

const int PLATFORMS_MIN = 3, PLATFORMS_MAX = 8;

const int COL_DEFAULT = 7,
          COL_BORDER = 8,
          COL_PLATFORM = 6,
          COL_PLAYER = 11,
          COL_ENEMY = 12,
          COL_BOSS = 13,
          COL_ATTACK = 10,
          COL_HP_FULL = 10,
          COL_HP_EMPTY = 8;

const char attackAnimations[4][ATTACK_CELLS] = {
    {'/', '-', '\\'},
    {'/', '|', '\\'},
    {'\\', '-', '/'},
    {'\\', '|', '/'}
};
char oldCellSymbol[ATTACK_CELLS];

int hitpoints = INITIAL_HITPOINTS;
int playerX = (WIDTH / 2) - 1;
int playerY = (HEIGHT / 2) - 1;

int jumpsLeft = 2;
int velX = 0, velY = 0;

Enemy* enemies = nullptr;
int currentWave = 0;
int enemiesCount = 0;

char** gameMatrix = nullptr;

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

void render() {
    moveCursor(0, 0);

    cout << "HP: ";
    for (int i = 1; i <= INITIAL_HITPOINTS; i++) {
        if (i <= hitpoints) SetConsoleTextAttribute(hConsole, COL_HP_FULL);
        else SetConsoleTextAttribute(hConsole, COL_HP_EMPTY);

        cout << (i <= hitpoints ? '0' : 'o');

        SetConsoleTextAttribute(hConsole, COL_DEFAULT);
        if (i < INITIAL_HITPOINTS) cout << '-';
    }

    SetConsoleTextAttribute(hConsole, COL_DEFAULT);
    cout << '\t'
        << "- (a / d move, w jump, double jump, i / j / k / l attack)"
        << endl;

    for (int i = 0; i < enemiesCount; i++) {
        if (enemies[i].alive)
            gameMatrix[enemies[i].y][enemies[i].x] = enemies[i].type;
        else
            gameMatrix[enemies[i].y][enemies[i].x] = EMPTY;
    }

    int currentColor = COL_DEFAULT;
    SetConsoleTextAttribute(hConsole, currentColor);

    for (int i = 0; i < HEIGHT; i++) {
        for (int j = 0; j < WIDTH; j++) {

            char ch = gameMatrix[i][j];

            if (i == playerY && j == playerX) ch = PLAYER;

            int wantedColor = COL_DEFAULT;

            switch (ch) {
                case BORDER:
                    wantedColor = COL_BORDER;
                    break;
                case PLATFORM:
                    wantedColor = COL_PLATFORM;
                    break;
                case PLAYER:
                    wantedColor = COL_PLAYER;
                    break;
                case BASIC_WALKER:
                case JUMPER:
                case FLIER:
                case CRAWLER:
                    wantedColor = COL_ENEMY;
                    break;
                case BOSS:
                    wantedColor = COL_BOSS;
                    break;
                case '/':
                case '\\':
                case '-':
                case '|':
                    wantedColor = COL_ATTACK;
                    break;
                default:
                    wantedColor = COL_DEFAULT;
                    break;
            }

            if (wantedColor != currentColor) {
                currentColor = wantedColor;
                SetConsoleTextAttribute(hConsole, currentColor);
            }

            cout << ch;
        }

        if (currentColor != COL_DEFAULT) {
            currentColor = COL_DEFAULT;
            SetConsoleTextAttribute(hConsole, currentColor);
        }
        cout << endl;
    }

    SetConsoleTextAttribute(hConsole, COL_DEFAULT);
}

void generatePlatform(int posX, int posY, int length) {
    if (posY < 1 || posY > HEIGHT - 2) return;

    for (int i = 0; i < length; i++) {
        int px = posX + i;
        if (px < 1 || px > WIDTH - 2) break;

        if (gameMatrix[posY][px] == EMPTY)
            gameMatrix[posY][px] = PLATFORM;
    }
}

void generatePlatforms(int platformsCount) {
    generatePlatform(playerX - 2, playerY + 1, 5);

    int x = randInt(8, 16);
    int y = playerY + 1;

    for (int i = 1; i < platformsCount; i++) {
        int length = randInt(6, 14);
        generatePlatform(x, y, length);

        int dx = randInt(8, 16);
        int dy = randInt(-2, 2) * JUMP_STRENGTH;

        y += dy;
        if (y < 2) y = 2;
        if (y > HEIGHT - 3) y = HEIGHT - 3;

        x += dx;
        if (x > WIDTH - 20) break;
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

void attack(Direction dir) {
    const char* attackAnimation = attackAnimations[dir];

    int attackCellX[ATTACK_CELLS], attackCellY[ATTACK_CELLS];

    if (dir == Up) {
        attackCellX[0] = playerX - 1;
        attackCellX[1] = playerX;
        attackCellX[2] = playerX + 1;

        attackCellY[0] = playerY - 1;
        attackCellY[1] = playerY - 1;
        attackCellY[2] = playerY - 1;
    }
    else if (dir == Left) {
        attackCellX[0] = playerX - 1;
        attackCellX[1] = playerX - 1;
        attackCellX[2] = playerX - 1;

        attackCellY[0] = playerY - 1;
        attackCellY[1] = playerY;
        attackCellY[2] = playerY + 1;
    }
    else if (dir == Down) {
        attackCellX[0] = playerX - 1;
        attackCellX[1] = playerX;
        attackCellX[2] = playerX + 1;

        attackCellY[0] = playerY + 1;
        attackCellY[1] = playerY + 1;
        attackCellY[2] = playerY + 1;
    }
    else {
        attackCellX[0] = playerX + 1;
        attackCellX[1] = playerX + 1;
        attackCellX[2] = playerX + 1;

        attackCellY[0] = playerY - 1;
        attackCellY[1] = playerY;
        attackCellY[2] = playerY + 1;
    }

    for (int i = 0; i < enemiesCount; i++) {
        if (!enemies[i].alive) continue;

        for (int j = 0; j < ATTACK_CELLS; j++) {
            if (enemies[i].x == attackCellX[j] &&
                enemies[i].y == attackCellY[j]) {
                enemies[i].alive = false;
            }
        }
    }

    for (int i = 0; i < ATTACK_CELLS; i++) {
        int currentX = attackCellX[i];
        int currentY = attackCellY[i];

        if (currentX < 1 || currentX > WIDTH - 2 || currentY < 1 || currentY > HEIGHT - 2) {
            oldCellSymbol[i] = 0;
            continue;
        }

        if (gameMatrix[currentY][currentX] == BORDER || gameMatrix[currentY][currentX] == PLATFORM) {
            oldCellSymbol[i] = 0;
            continue;
        }

        oldCellSymbol[i] = gameMatrix[currentY][currentX];
        gameMatrix[currentY][currentX] = attackAnimation[i];
    }

    render();
    Sleep(70);

    for (int i = 0; i < ATTACK_CELLS; i++) {
        if (oldCellSymbol[i] == 0) continue;

        int currentX = attackCellX[i];
        int currentY = attackCellY[i];

        gameMatrix[currentY][currentX] = oldCellSymbol[i];
    }
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
            case 'I':
            case 'i':
                attack(Up);
                break;
            case 'J':
            case 'j':
                attack(Left);
                break;
            case 'K':
            case 'k':
                attack(Down);
                break;
            case 'L':
            case 'l':
                attack(Right);
                break;
        }
    }
}

void gravityCheck() {
    bool onGround = isBlocked(playerX, playerY + 1);

    if (onGround && velY >= 0) {
        velY = 0;
        jumpsLeft = 2;
    }

    int steps = abs(velY);
    int dir = (velY < 0) ? -1 : 1;

    for (int i = 0; i < steps; i++) {
        int newY = playerY + dir;

        if (newY < 1 || newY > HEIGHT - 2) { velY = 0; break; }
        if (isBlocked(playerX, newY)) { velY = 0; break; }

        playerY = newY;
    }

    onGround = isBlocked(playerX, playerY + 1);
    if (!onGround) velY += GRAVITY;
}

char randEnemyType() {
    int randEnemyCode = randInt(1, 4);

    switch (randEnemyCode) {
        case 1: return BASIC_WALKER;
        case 2: return JUMPER;
        case 3: return FLIER;
        case 4: return CRAWLER;
        default: return BASIC_WALKER;
    }
}

void startWave(int wave) {
    delete[] enemies;

    int platformsCount = randInt(PLATFORMS_MIN, PLATFORMS_MAX);
    generatePlatforms(platformsCount);

    enemiesCount += randInt(2, 4);
    enemies = new Enemy[enemiesCount];

    for (int i = 0; i < enemiesCount; i++) {
        enemies[i].x = randInt(1, WIDTH - 2);
        enemies[i].y = randInt(1, HEIGHT - 3);
        enemies[i].type = randEnemyType();
        enemies[i].attackCooldown = ENEMY_ATTACK_COOLDOWN;
        enemies[i].alive = true;
    }
}

bool waveHasEnded() {
    for (int i = 0; i < enemiesCount; i++) {
        if (enemies[i].alive)
            return false;
    }

    return true;
}

void handleBasicWalker(Enemy& basicWalker) {
    int directionX = (playerX < basicWalker.x) ? -1 : 1;

    if (!isBlocked(basicWalker.x + directionX, basicWalker.y))
        basicWalker.x += directionX;
}

void handleJumper(Enemy& jumper) {
    int directionX = (playerX < jumper.x) ? -1 : 1;

    if (!isBlocked(jumper.x + directionX, jumper.y))
        jumper.x += directionX;

    if (abs(playerX - jumper.x) <= 2 && !isBlocked(jumper.x, jumper.y - 1))
        jumper.y -= 1;
}


void handleFlier(Enemy& flier) {
    if (playerX < flier.x) flier.x--;
    if (playerX > flier.x) flier.x++;

    if (playerY < flier.y) flier.y--;
    if (playerY > flier.y) flier.y++;
}

void handleCrawler(Enemy& crawler) {
    static int direction = 1;

    if (isBlocked(crawler.x + direction, crawler.y))
        direction = -direction;

    if (!isBlocked(crawler.x + direction, crawler.y))
        crawler.x += direction;
}

void handleEnemies() {
    for (int i = 0; i < enemiesCount; i++) {
        if (!enemies[i].alive)
            continue;

        if (enemies[i].attackCooldown > 0)
            enemies[i].attackCooldown--;

        switch (enemies[i].type) {
            case BASIC_WALKER:
                handleBasicWalker(enemies[i]);
                break;
            case JUMPER:
                handleJumper(enemies[i]);
                break;
            case FLIER:
                handleFlier(enemies[i]);
                break;
            case CRAWLER:
                handleCrawler(enemies[i]);
                break;
        }

        if (enemies[i].attackCooldown > 0) 

        if (enemies[i].x == playerX && enemies[i].y == playerY) {
            hitpoints--;
            enemies[i].attackCooldown = ENEMY_ATTACK_COOLDOWN;
        }
    }
}

bool gameHasEnded() {
    if (hitpoints <= 0) {
        moveCursor(0, 0);
        render();

        SetConsoleTextAttribute(hConsole, 12);
        cout << "GAME OVER\n";
        SetConsoleTextAttribute(hConsole, COL_DEFAULT);

        return true;
    }

    if (currentWave >= WAVES_NUMBER && waveHasEnded()) {
        moveCursor(0, 0);
        render();

        SetConsoleTextAttribute(hConsole, 10);
        cout << "YOU WIN\n";
        SetConsoleTextAttribute(hConsole, COL_DEFAULT);

        return true;
    }

    return false;
}

int main()
{
    srand((unsigned)time(nullptr));

    hideCursor();

    initializeGameMatrix(WIDTH, HEIGHT);

    while (true) {
        handleInput();
        gravityCheck();
        handleEnemies();

        if (waveHasEnded())
            startWave(++currentWave);

        if (gameHasEnded())
            break;

        render();
        Sleep(40);
    }

    cout << "Press any key to exit...";

    _getch();
}
