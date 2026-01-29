/**
*
* Solution to course project #10
* Introduction to programming course
* Faculty of Mathematics and Informatics of Sofia University
* Winter semester 2025/2026
*
* Simeon Gunev
* 0MI0600562
* VC
*
* ASCII Knight is a console-based game project developed as part of the Introduction to Programming (UP Practicum) course.
*
*/

#include <iostream>
#include <conio.h>
#include <cstdlib>
#include <ctime>
#include <windows.h>

using namespace std;

struct Enemy {
	int x;
	int y;
	char type;
	int hp;
	int attackCooldown;
	bool alive;
	int dir;
	int timer;
};

enum Direction {
	Up,
	Left,
	Down,
	Right
};

const int WIDTH = 90;
const int HEIGHT = 25;

const int INITIAL_HITPOINTS = 5;
const int WAVES_NUMBER = 3;

const char PLAYER = '@';
const char BASIC_WALKER = 'E';
const char JUMPER = 'J';
const char FLIER = 'F';
const char CRAWLER = 'C';
const char BOSS = 'B';

const char EMPTY = ' ';
const char PLATFORM = '=';
const char BORDER = '#';

const int JUMP_STRENGTH = 2;
const int GRAVITY = 1;

const int ATTACK_CELLS = 3;
const int ENEMY_ATTACK_COOLDOWN = 20;
const int PLAYER_ATTACK_COOLDOWN = 8;

const int PLATFORMS_MIN = 3;
const int PLATFORMS_MAX = 8;

const int WAVE_BREAK_TICKS = 75;
const int TICKS_PER_SECOND = 25;

const int COL_DEFAULT = 7;
const int COL_BORDER = 8;
const int COL_PLATFORM = 6;
const int COL_PLAYER = 11;
const int COL_ENEMY = 12;
const int COL_BOSS = 13;
const int COL_ATTACK = 10;
const int COL_HP_FULL = 10;
const int COL_HP_EMPTY = 8;

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
int velY = 0;

int playerAttackCooldown = 0;

Enemy* enemies = nullptr;
int enemiesCount = 0;

int currentWave = 1;
int plannedEnemiesForNextWave = 0;

bool enemiesSpawnedThisWave = false;
int waveBreakTicks = WAVE_BREAK_TICKS;

char enemyMap[HEIGHT][WIDTH];
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

bool isInsideArena(int x, int y) {
	if (x < 1 || x > WIDTH - 2 || y < 1 || y > HEIGHT - 2) {
		return false;
	}

	return true;
}

void clearEnemyMap() {
	for (int i = 0; i < HEIGHT; i++) {
		for (int j = 0; j < WIDTH; j++) {
			enemyMap[i][j] = 0;
		}
	}
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
	}
}

void resetArenaInsideToEmpty() {
	for (int i = 1; i < HEIGHT - 1; i++) {
		for (int j = 1; j < WIDTH - 1; j++) {
			gameMatrix[i][j] = EMPTY;
		}
	}
}

bool isBlocked(int x, int y) {
	if (gameMatrix[y][x] == BORDER)
		return true;

	if (gameMatrix[y][x] == PLATFORM)
		return true;

	if (enemyMap[y][x] != 0)
		return true;

	return false;
}

void generatePlatform(int x, int y, int length) {
	if (y < 1) {
		return;
	}
	if (y > HEIGHT - 2) {
		return;
	}

	for (int i = 0; i < length; i++) {
		int px = x + i;

		if (px < 1) {
			break;
		}
		if (px > WIDTH - 2) {
			break;
		}

		if (gameMatrix[y][px] == EMPTY) {
			gameMatrix[y][px] = PLATFORM;
		}
	}
}

void generatePlatforms(int platformsCount) {
	generatePlatform(playerX - 2, playerY + 1, 7);

	int x = randInt(8, 16);
	int y = playerY + 1;

	for (int i = 1; i < platformsCount; i++) {
		int length = randInt(6, 14);
		generatePlatform(x, y, length);

		int dx = randInt(8, 16);

		int dy = 2;
		if (randInt(0, 1) == 1) {
			dy = 4;
		}

		int sign = 1;
		if (randInt(0, 1) == 0) {
			sign = -1;
		}

		y = y + sign * dy;

		if (y < 2) {
			y = 2;
		}
		if (y > HEIGHT - 3) {
			y = HEIGHT - 3;
		}

		x = x + dx;

		if (x > WIDTH - 20) {
			break;
		}
	}
}

void rebuildEnemyMapFromEnemies() {
	clearEnemyMap();

	if (enemies == nullptr) {
		return;
	}

	for (int i = 0; i < enemiesCount; i++) {
		if (!enemies[i].alive) {
			continue;
		}

		if (enemies[i].type == BOSS) {
			for (int dy = 0; dy < 3; dy++) {
				for (int dx = 0; dx < 3; dx++) {
					int xx = enemies[i].x + dx;
					int yy = enemies[i].y + dy;

					if (isInsideArena(xx, yy)) {
						enemyMap[yy][xx] = BOSS;
					}
				}
			}
		}
		else {
			if (isInsideArena(enemies[i].x, enemies[i].y)) {
				enemyMap[enemies[i].y][enemies[i].x] = enemies[i].type;
			}
		}
	}
}

void render() {
	moveCursor(0, 0);

	cout << "HP: ";

	for (int i = 1; i <= INITIAL_HITPOINTS; i++) {
		if (i <= hitpoints) {
			SetConsoleTextAttribute(hConsole, COL_HP_FULL);
		}
		else {
			SetConsoleTextAttribute(hConsole, COL_HP_EMPTY);
		}

		if (i <= hitpoints) {
			cout << '0';
		}
		else {
			cout << 'o';
		}

		SetConsoleTextAttribute(hConsole, COL_DEFAULT);

		if (i < INITIAL_HITPOINTS) {
			cout << '-';
		}
	}

	SetConsoleTextAttribute(hConsole, COL_DEFAULT);
	cout << "   Wave: " << currentWave << "/" << WAVES_NUMBER;
	cout << '\t' << "- (a / d move, w jump, double jump, i / j / k / l attack)" << endl;

	int currentColor = COL_DEFAULT;
	SetConsoleTextAttribute(hConsole, currentColor);

	for (int i = 0; i < HEIGHT; i++) {
		for (int j = 0; j < WIDTH; j++) {
			char ch = gameMatrix[i][j];

			if (i == playerY && j == playerX) {
				ch = PLAYER;
			}

			if (enemyMap[i][j] != 0) {
				ch = enemyMap[i][j];
			}

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

	moveCursor(0, HEIGHT + 1);

	if (!enemiesSpawnedThisWave) {
		int secondsLeft = (waveBreakTicks + (TICKS_PER_SECOND - 1)) / TICKS_PER_SECOND;
		cout << "Wave starting in: " << secondsLeft << "s";
	}
}

void moveHorizontal(int dx) {
	int newX = playerX + dx;

	if (newX < 1) {
		return;
	}
	if (newX > WIDTH - 2) {
		return;
	}
	if (isBlocked(newX, playerY)) {
		return;
	}

	playerX = newX;
}

void jump() {
	if (jumpsLeft <= 0) {
		return;
	}

	velY = -JUMP_STRENGTH;
	jumpsLeft--;
}

void gravityCheckPlayer() {
	bool onGround = isBlocked(playerX, playerY + 1);

	if (onGround && velY >= 0) {
		velY = 0;
		jumpsLeft = 2;
	}

	int steps = abs(velY);
	int dir = (velY < 0) ? -1 : 1;

	for (int i = 0; i < steps; i++) {
		int newY = playerY + dir;

		if (!isInsideArena(playerX, newY) || isBlocked(playerX, newY)) {
			velY = 0;
			break;
		}

		playerY = newY;
	}

	if (!isBlocked(playerX, playerY + 1)) {
		velY += GRAVITY;
	}
}

bool playerHitsBossCell(Enemy& boss, int x, int y) {
	return (x >= boss.x && x <= boss.x + 2 && y >= boss.y && y <= boss.y + 2);
}

void attack(Direction dir) {
	if (playerAttackCooldown > 0) {
		return;
	}

	playerAttackCooldown = PLAYER_ATTACK_COOLDOWN;

	const char* anim = attackAnimations[dir];

	int ax[ATTACK_CELLS];
	int ay[ATTACK_CELLS];

	switch (dir) {
	case Up:
		ax[0] = playerX - 1;
		ax[1] = playerX;
		ax[2] = playerX + 1;

		ay[0] = playerY - 1;
		ay[1] = playerY - 1;
		ay[2] = playerY - 1;
		break;

	case Left:
		ax[0] = playerX - 1;
		ax[1] = playerX - 1;
		ax[2] = playerX - 1;

		ay[0] = playerY - 1;
		ay[1] = playerY;
		ay[2] = playerY + 1;
		break;

	case Down:
		ax[0] = playerX - 1;
		ax[1] = playerX;
		ax[2] = playerX + 1;

		ay[0] = playerY + 1;
		ay[1] = playerY + 1;
		ay[2] = playerY + 1;
		break;

	case Right:
		ax[0] = playerX + 1;
		ax[1] = playerX + 1;
		ax[2] = playerX + 1;

		ay[0] = playerY - 1;
		ay[1] = playerY;
		ay[2] = playerY + 1;
		break;
	}

	if (enemies != nullptr) {
		for (int i = 0; i < enemiesCount; i++) {
			if (!enemies[i].alive) {
				continue;
			}

			if (enemies[i].type == BOSS) {
				for (int j = 0; j < ATTACK_CELLS; j++) {
					if (playerHitsBossCell(enemies[i], ax[j], ay[j])) {
						enemies[i].hp--;

						if (enemies[i].hp <= 0) {
							enemies[i].alive = false;
						}

						break;
					}
				}
			}
			else {
				for (int j = 0; j < ATTACK_CELLS; j++) {
					if (enemies[i].x != ax[j]) {
						continue;
					}
					if (enemies[i].y != ay[j]) {
						continue;
					}

					enemies[i].alive = false;
					break;
				}
			}
		}
	}

	for (int i = 0; i < ATTACK_CELLS; i++) {
		int x = ax[i];
		int y = ay[i];

		if (!isInsideArena(x, y)) {
			oldCellSymbol[i] = 0;
			continue;
		}

		if (gameMatrix[y][x] == BORDER) {
			oldCellSymbol[i] = 0;
			continue;
		}

		if (gameMatrix[y][x] == PLATFORM) {
			oldCellSymbol[i] = 0;
			continue;
		}

		oldCellSymbol[i] = gameMatrix[y][x];
		gameMatrix[y][x] = anim[i];
	}

	rebuildEnemyMapFromEnemies();
	render();
	Sleep(70);

	for (int i = 0; i < ATTACK_CELLS; i++) {
		if (oldCellSymbol[i] == 0) {
			continue;
		}

		int x = ax[i];
		int y = ay[i];

		if (isInsideArena(x, y)) {
			gameMatrix[y][x] = oldCellSymbol[i];
		}
	}
}

void handleInput() {
	if (!_kbhit()) {
		return;
	}

	char ch = (char)_getch();

	switch (ch) {
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

char randEnemyTypeNonBoss() {
	int r = randInt(1, 4);

	switch (r) {
	case 1:
		return BASIC_WALKER;
	case 2:
		return JUMPER;
	case 3:
		return FLIER;
	case 4:
		return CRAWLER;
	}

	return BASIC_WALKER;
}

bool findSpawnCell(int& outX, int& outY, char type) {
	for (int t = 0; t < 300; t++) {
		int x = randInt(5, WIDTH - 6);
		int y = randInt(2, HEIGHT - 4);

		if (type == FLIER || type == CRAWLER) {
			if (gameMatrix[y][x] == EMPTY && (abs(x - playerX) > 10)) {
				outX = x; outY = y;
				return true;
			}
			continue;
		}

		if (gameMatrix[y][x] == EMPTY &&
			(gameMatrix[y + 1][x] == PLATFORM || gameMatrix[y + 1][x] == BORDER)) {

			if (abs(x - playerX) > 8) {
				outX = x; outY = y;
				return true;
			}
		}
	}

	outX = (playerX < WIDTH / 2) ? WIDTH - 10 : 10;
	outY = HEIGHT - 2;
	return true;
}

void spawnEnemiesForWave(int wave) {
	delete[] enemies;
	enemies = nullptr;
	enemiesCount = 0;

	if (wave == WAVES_NUMBER) {
		enemiesCount = 1;
		enemies = new Enemy[enemiesCount];

		enemies[0].type = BOSS;
		enemies[0].hp = 6;
		enemies[0].attackCooldown = 28;
		enemies[0].alive = true;
		enemies[0].dir = 0;
		enemies[0].timer = 0;

		enemies[0].x = WIDTH - 5;
		enemies[0].y = 2;
	}
	else {
		if (plannedEnemiesForNextWave == 0) {
			plannedEnemiesForNextWave = randInt(2, 4);
		}
		else {
			plannedEnemiesForNextWave = plannedEnemiesForNextWave + randInt(2, 3);
		}

		enemiesCount = plannedEnemiesForNextWave;
		enemies = new Enemy[enemiesCount];

		for (int i = 0; i < enemiesCount; i++) {
			enemies[i].alive = true;
			enemies[i].attackCooldown = ENEMY_ATTACK_COOLDOWN;
			enemies[i].hp = 1;

			if (randInt(0, 1) == 0) {
				enemies[i].dir = -1;
			}
			else {
				enemies[i].dir = 1;
			}

			enemies[i].timer = randInt(0, 20);

			int sx = 0;
			int sy = 0;
			findSpawnCell(sx, sy, enemies[i].type);

			enemies[i].x = sx;
			enemies[i].y = sy;

			if (wave == 1 && i == 0) {
				enemies[i].type = BASIC_WALKER;
			}
			else if (wave == 2 && i == 0) {
				enemies[i].type = JUMPER;
			}
			else {
				enemies[i].type = randEnemyTypeNonBoss();
			}
		}
	}

	rebuildEnemyMapFromEnemies();
}

void startWaveArenaOnly() {
	resetArenaInsideToEmpty();

	int platformsCount = randInt(PLATFORMS_MIN, PLATFORMS_MAX);
	generatePlatforms(platformsCount);

	clearEnemyMap();
}

bool allEnemiesDead() {
	if (enemies == nullptr) {
		return true;
	}

	for (int i = 0; i < enemiesCount; i++) {
		if (enemies[i].alive) {
			return false;
		}
	}

	return true;
}

void applyEnemyGravity(Enemy& e) {
	switch (e.type) {
	case FLIER:
		return;

	case BOSS:
	{
		bool blocked = false;
		int belowY = e.y + 3;

		for (int dx = 0; dx < 3; dx++) {
			int xx = e.x + dx;

			if (!isInsideArena(xx, belowY)) {
				blocked = true;
				break;
			}

			if (isBlocked(xx, belowY)) {
				blocked = true;
				break;
			}
		}

		if (!blocked) {
			e.y = e.y + 1;
		}

		return;
	}
	}

	if (!isBlocked(e.x, e.y + 1)) {
		e.y = e.y + 1;
	}
}

void handleBasicWalker(Enemy& e) {
	int detectionRange = 15;
	int dist = abs(playerX - e.x);

	if (dist < detectionRange) {
		int dx = (playerX < e.x) ? -1 : 1;
		if (!isBlocked(e.x + dx, e.y)) {
			e.x += dx;
		}
	}
	else {
		if (e.timer % 20 == 0) {
			if (randInt(0, 5) == 0) e.dir *= -1;
		}

		if (!isBlocked(e.x + e.dir, e.y)) {
			e.x += e.dir;
		}
		else {
			e.dir *= -1;
		}
	}
	e.timer++;
	applyEnemyGravity(e);
}

void handleJumper(Enemy& e) {
	bool onGround = isBlocked(e.x, e.y + 1);
	int dx = (playerX < e.x) ? -1 : 1;

	if (e.timer % 15 == 0 && !isBlocked(e.x + dx, e.y)) {
		e.x += dx;
	}

	if (onGround) {
		bool shouldJump = (abs(playerX - e.x) <= 4 && abs(playerY - e.y) <= 3) || isBlocked(e.x + dx, e.y);
		if (shouldJump && randInt(0, 3) == 0) {
			if (!isBlocked(e.x, e.y - 1)) e.y -= 1;
			if (!isBlocked(e.x, e.y - 1)) e.y -= 1;
		}
	}

	e.timer++;
	applyEnemyGravity(e);
}

void handleFlier(Enemy& e) {
	if (e.timer % 2 == 0) {
		if (playerX < e.x && !isBlocked(e.x - 1, e.y)) e.x--;
		else if (playerX > e.x && !isBlocked(e.x + 1, e.y)) e.x++;
	}

	int targetY = playerY - 2;
	if (e.timer % 4 == 0) {
		if (e.y < targetY && !isBlocked(e.x, e.y + 1)) e.y++;
		else if (e.y > targetY && !isBlocked(e.x, e.y - 1)) e.y--;
	}

	if (randInt(0, 10) == 0) {
		int dy = (randInt(0, 1) == 0) ? 1 : -1;
		if (!isBlocked(e.x, e.y + dy)) e.y += dy;
	}

	e.timer++;
}

void handleCrawler(Enemy& e) {
	if (e.dir == 0) e.dir = 1;

	int nx = e.x + e.dir;

	if (isBlocked(nx, e.y)) {
		if (e.y > 1 && !isBlocked(e.x, e.y - 1)) {
			e.y = e.y - 1;
		}
		else {
			e.dir = -e.dir;
		}
	}
	else {
		e.x = nx;
	}

	if (e.type != BOSS && !isBlocked(e.x, e.y + 1)) {
		e.y = e.y + 1;
	}

	if (e.x < 1) e.x = 1;
	if (e.x > WIDTH - 2) e.x = WIDTH - 2;
	if (e.y < 1) e.y = 1;
	if (e.y > HEIGHT - 2) e.y = HEIGHT - 2;
}

void bossAttackLine(Enemy& boss) {
	int cx = boss.x + 1;
	int cy = boss.y + 1;

	int vx = playerX - cx;
	int vy = playerY - cy;

	int dx = 0;
	int dy = 0;

	if (abs(vx) >= abs(vy)) {
		if (vx < 0) {
			dx = -1;
		}
		else {
			dx = 1;
		}
	}
	else {
		if (vy < 0) {
			dy = -1;
		}
		else {
			dy = 1;
		}
	}

	int range = 8;

	for (int i = 1; i <= range; i++) {
		int x = cx + dx * i;
		int y = cy + dy * i;

		if (!isInsideArena(x, y))
			break;

		if (gameMatrix[y][x] == BORDER)
			break;

		if (gameMatrix[y][x] == PLATFORM)
			break;

		if (enemyMap[y][x] != 0) 
			break;

		char sym = '|';
		if (dx != 0) {
			sym = '-';
		}

		char old = gameMatrix[y][x];
		gameMatrix[y][x] = sym;

		if (playerX == x && playerY == y) {
			hitpoints = hitpoints - 2;
		}

		rebuildEnemyMapFromEnemies();
		render();
		Sleep(15);

		gameMatrix[y][x] = old;
	}
}

void handleBoss(Enemy& boss) {
	boss.timer++;

	int speed = (boss.hp < 3) ? 2 : 3;
	if (boss.timer % speed == 0) {
		if (playerX < boss.x + 1 && boss.x > 1) boss.x--;
		else if (playerX > boss.x + 1 && boss.x < WIDTH - 4) boss.x++;
	}

	if (boss.attackCooldown > 0) boss.attackCooldown--;
	else {
		boss.attackCooldown = (boss.hp < 3) ? 15 : 28;
		bossAttackLine(boss);
	}

	applyEnemyGravity(boss);

	if (playerHitsBossCell(boss, playerX, playerY)) {
		if (boss.timer % 10 == 0) hitpoints -= (boss.hp < 3) ? 2 : 1;
	}
}

void handleEnemies() {
	if (enemies == nullptr) {
		return;
	}

	if (playerAttackCooldown > 0) {
		playerAttackCooldown = playerAttackCooldown - 1;
	}

	for (int i = 0; i < enemiesCount; i++) {
		Enemy& enemy = enemies[i];

		if (!enemy.alive) {
			continue;
		}

		switch (enemy.type) {
		case BASIC_WALKER:
			handleBasicWalker(enemy);
			break;
		case JUMPER:
			handleJumper(enemy);
			break;
		case FLIER:
			handleFlier(enemy);
			break;
		case CRAWLER:
			handleCrawler(enemy);
			break;
		case BOSS:
			handleBoss(enemy);
			break;
		}

		for (int j = 0; j < enemiesCount; j++) {
			if (i == j || !enemies[j].alive) continue;

			if (enemy.x == enemies[j].x && enemy.y == enemies[j].y && enemy.type != BOSS && enemies[j].type != BOSS) {
				int pushDir = (enemy.x < enemies[j].x) ? -1 : 1;

				if (enemy.x == enemies[j].x) pushDir = (enemy.x > WIDTH / 2) ? -1 : 1;

				if (!isBlocked(enemy.x + pushDir, enemy.y)) {
					enemy.x += pushDir;
				}

				if (enemy.type == BASIC_WALKER || enemy.type == CRAWLER) {
					enemy.dir = -enemy.dir;
				}
				if (enemies[j].type == BASIC_WALKER || enemies[j].type == CRAWLER) {
					enemies[j].dir = -enemies[j].dir;
				}
			}
		}

		if (enemy.type != BOSS) {
			if (enemy.x < 1) {
				enemy.x = 1;
			}
			if (enemy.x > WIDTH - 2) {
				enemy.x = WIDTH - 2;
			}
			if (enemy.y < 1) {
				enemy.y = 1;
			}
			if (enemy.y > HEIGHT - 2) {
				enemy.y = HEIGHT - 2;
			}
		}

		if (enemy.type != BOSS) {
			if (enemy.attackCooldown > 0) enemy.attackCooldown--;

			if (enemy.x == playerX && enemy.y == playerY) {
				if (enemy.attackCooldown <= 0) {
					hitpoints--;
					enemy.attackCooldown = ENEMY_ATTACK_COOLDOWN;

					if (playerX > enemy.x)
						moveHorizontal(1);
					else
						moveHorizontal(-1);
				}
			}
		}
	}

	rebuildEnemyMapFromEnemies();
}

bool gameHasEnded() {
	if (hitpoints <= 0) {
		return true;
	}

	if (currentWave == WAVES_NUMBER &&
		enemiesSpawnedThisWave &&
		allEnemiesDead()) {
		return true;
	}

	return false;
}

void showFinalMessage() {
	moveCursor(0, HEIGHT + 2);

	if (hitpoints <= 0) {
		SetConsoleTextAttribute(hConsole, 12);
		cout << "GAME OVER";
		SetConsoleTextAttribute(hConsole, COL_DEFAULT);
	}
	else {
		SetConsoleTextAttribute(hConsole, 10);
		cout << "YOU WIN";
		SetConsoleTextAttribute(hConsole, COL_DEFAULT);
	}
}

void prepareWaveArena() {
	int platformsCount = randInt(PLATFORMS_MIN, PLATFORMS_MAX);

	resetArenaInsideToEmpty();
	generatePlatforms(platformsCount);
	clearEnemyMap();
}

void clearEnemiesMemory() {
	if (enemies != nullptr) {
		delete[] enemies;
		enemies = nullptr;
	}

	enemiesCount = 0;
	clearEnemyMap();
}

int main() {
	srand((unsigned)time(nullptr));
	hideCursor();

	initializeGameMatrix(WIDTH, HEIGHT);

	prepareWaveArena();

	waveBreakTicks = WAVE_BREAK_TICKS;
	enemiesSpawnedThisWave = false;

	while (true) {
		handleInput();
		gravityCheckPlayer();

		if (!enemiesSpawnedThisWave) {
			clearEnemiesMemory();

			waveBreakTicks = waveBreakTicks - 1;

			if (waveBreakTicks <= 0) {
				spawnEnemiesForWave(currentWave);
				enemiesSpawnedThisWave = true;
			}
		}
		else {
			handleEnemies();

			if (allEnemiesDead() && currentWave < WAVES_NUMBER) {
				currentWave = currentWave + 1;

				enemiesSpawnedThisWave = false;
				waveBreakTicks = WAVE_BREAK_TICKS;

				prepareWaveArena();
			}
		}

		render();

		if (gameHasEnded()) {
			break;
		}

		Sleep(40);
	}

	enemiesSpawnedThisWave = true;
	render();
	showFinalMessage();

	moveCursor(0, HEIGHT + 3);
	cout << "Press any key to exit...";

	_getch();

	clearEnemiesMemory();

	if (gameMatrix != nullptr) {
		for (int i = 0; i < HEIGHT; i++) {
			delete[] gameMatrix[i];
		}
		delete[] gameMatrix;
		gameMatrix = nullptr;
	}

	return 0;
}
