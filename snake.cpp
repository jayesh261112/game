#include <SDL2/SDL.h>
#include <vector>
#include <string>
#include <cmath>
#include <cstdlib>
#include <ctime>

const int GRID = 20;
const int CELL = 32;

struct Point { int x, y; };

bool isOnSnake(const std::vector<Point>& snake, int x, int y) {
    for (const auto& p : snake)
        if (p.x == x && p.y == y) return true;
    return false;
}

Point spawnFood(const std::vector<Point>& snake) {
    Point f;
    do { f = { rand() % GRID, rand() % GRID }; } while (isOnSnake(snake, f.x, f.y));
    return f;
}

void resetGame(std::vector<Point>& snake, Point& food, int& dx, int& dy,
               int& nextDx, int& nextDy, int& score, bool& gameOver) {
    snake = { {10, 10}, {9, 10}, {8, 10}, {7, 10} };
    dx = nextDx = 1;
    dy = nextDy = 0;
    score = 0;
    gameOver = false;
    food = spawnFood(snake);
}

// ---------- drawing helpers ----------
void setColor(SDL_Renderer* r, int R, int G, int B) {
    SDL_SetRenderDrawColor(r, (Uint8)R, (Uint8)G, (Uint8)B, 255);
}

void fillCircle(SDL_Renderer* r, int cx, int cy, int rad) {
    for (int y = -rad; y <= rad; ++y) {
        int w = (int)std::sqrt((double)(rad * rad - y * y));
        SDL_RenderDrawLine(r, cx - w, cy + y, cx + w, cy + y);
    }
}

void drawBackground(SDL_Renderer* r) {
    for (int y = 0; y < GRID; ++y)
        for (int x = 0; x < GRID; ++x) {
            if ((x + y) % 2 == 0) setColor(r, 38, 120, 60);
            else                  setColor(r, 44, 132, 68);
            SDL_Rect rect = { x * CELL, y * CELL, CELL, CELL };
            SDL_RenderFillRect(r, &rect);
        }
}

void drawApple(SDL_Renderer* r, Point f) {
    int cx = f.x * CELL + CELL / 2;
    int cy = f.y * CELL + CELL / 2 + 2;
    setColor(r, 0, 0, 0);                       // soft shadow
    setColor(r, 30, 90, 45);
    fillCircle(r, cx + 2, cy + 3, CELL * 38 / 100);
    setColor(r, 220, 40, 45);                   // apple body
    fillCircle(r, cx, cy, CELL * 38 / 100);
    setColor(r, 255, 140, 140);                 // shine
    fillCircle(r, cx - 5, cy - 5, 3);
    setColor(r, 110, 70, 30);                   // stem
    SDL_Rect stem = { cx - 1, cy - CELL * 38 / 100 - 4, 3, 7 };
    SDL_RenderFillRect(r, &stem);
    setColor(r, 90, 200, 80);                   // leaf
    fillCircle(r, cx + 5, cy - CELL * 38 / 100 - 2, 4);
}

void drawSnake(SDL_Renderer* r, const std::vector<Point>& s, int dx, int dy, bool dead) {
    int n = (int)s.size();

    // body, from tail to head so the head ends up on top
    for (int i = n - 1; i >= 1; --i) {
        float t = (float)i / (float)(n - 1);            // 0 = head, 1 = tail
        float rad = CELL * (0.46f - 0.16f * t);          // tapers toward the tail
        int cx = s[i].x * CELL + CELL / 2;
        int cy = s[i].y * CELL + CELL / 2;

        int R = (int)(70 - 30 * t), G = (int)(200 - 80 * t), B = (int)(90 - 30 * t);
        if (dead) { R = 130; G = 130; B = 130; }
        setColor(r, R, G, B);

        // join to the next segment so the body looks continuous
        if (i + 1 < n && std::abs(s[i].x - s[i + 1].x) <= 1 && std::abs(s[i].y - s[i + 1].y) <= 1) {
            int mx = (cx + s[i + 1].x * CELL + CELL / 2) / 2;
            int my = (cy + s[i + 1].y * CELL + CELL / 2) / 2;
            fillCircle(r, mx, my, (int)rad);
        }
        fillCircle(r, cx, cy, (int)rad);
        
        // scale pattern on every second segment
        if (i % 2 == 0) {
            setColor(r, dead ? 100 : R - 25, dead ? 100 : G - 40, dead ? 100 : B - 20);
            fillCircle(r, cx, cy, (int)(rad * 0.45f));
        }
    }

    // join head to first body segment
    if (n > 1 && std::abs(s[0].x - s[1].x) <= 1 && std::abs(s[0].y - s[1].y) <= 1) {
        int mx = (s[0].x * CELL + s[1].x * CELL) / 2 + CELL / 2;
        int my = (s[0].y * CELL + s[1].y * CELL) / 2 + CELL / 2;
        dead ? setColor(r, 130, 130, 130) : setColor(r, 70, 200, 90);
        fillCircle(r, mx, my, (int)(CELL * 0.44f));
    }

    // head
    int hx = s[0].x * CELL + CELL / 2;
    int hy = s[0].y * CELL + CELL / 2;
    int hr = (int)(CELL * 0.52f);

    // tongue (flickers on and off)
    if (!dead && (SDL_GetTicks() / 250) % 2 == 0) {
        setColor(r, 230, 40, 60);
        int bx = hx + dx * hr, by = hy + dy * hr;
        int tx = hx + dx * (hr + 9), ty = hy + dy * (hr + 9);
        SDL_RenderDrawLine(r, bx, by, tx, ty);
        SDL_RenderDrawLine(r, tx, ty, tx + dx * 4 - dy * 4, ty + dy * 4 + dx * 4);
        SDL_RenderDrawLine(r, tx, ty, tx + dx * 4 + dy * 4, ty + dy * 4 - dx * 4);
    }

    dead ? setColor(r, 150, 150, 150) : setColor(r, 90, 225, 120);
    fillCircle(r, hx, hy, hr);

    // eyes: forward along the direction, spread to both sides
    int px = -dy, py = dx;
    for (int side = -1; side <= 1; side += 2) {
        int ex = hx + dx * (CELL * 22 / 100) + px * side * (CELL * 26 / 100);
        int ey = hy + dy * (CELL * 22 / 100) + py * side * (CELL * 26 / 100);
        setColor(r, 255, 255, 255);
        fillCircle(r, ex, ey, 5);
        setColor(r, 10, 10, 10);
        if (dead) {
            SDL_RenderDrawLine(r, ex - 3, ey - 3, ex + 3, ey + 3);
            SDL_RenderDrawLine(r, ex - 3, ey + 3, ex + 3, ey - 3);
        } else {
            fillCircle(r, ex + dx * 2, ey + dy * 2, 2);
        }
    }
}

int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) return 1;
    srand((unsigned)time(nullptr));

    SDL_Window* window = SDL_CreateWindow("Snake",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        GRID * CELL, GRID * CELL, 0);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    std::vector<Point> snake;
    Point food;
    int dx, dy, nextDx, nextDy, score;
    bool gameOver;
    resetGame(snake, food, dx, dy, nextDx, nextDy, score, gameOver);

    bool running = true;
    Uint32 last = SDL_GetTicks();

    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = false;
            if (e.type == SDL_KEYDOWN) {
                switch (e.key.keysym.sym) {
                    case SDLK_UP:    if (dy != 1)  { nextDx = 0;  nextDy = -1; } break;
                    case SDLK_DOWN:  if (dy != -1) { nextDx = 0;  nextDy = 1;  } break;
                    case SDLK_LEFT:  if (dx != 1)  { nextDx = -1; nextDy = 0;  } break;
                    case SDLK_RIGHT: if (dx != -1) { nextDx = 1;  nextDy = 0;  } break;
                    case SDLK_SPACE:
                        if (gameOver)
                            resetGame(snake, food, dx, dy, nextDx, nextDy, score, gameOver);
                        break;
                    case SDLK_ESCAPE: running = false; break;
                }
            }
        }

        int delay = 150 - score * 4;
        if (delay < 60) delay = 60;

        if (!gameOver && SDL_GetTicks() - last > (Uint32)delay) {
            last = SDL_GetTicks();
            dx = nextDx;
            dy = nextDy;

            Point head = { (snake[0].x + dx + GRID) % GRID,
                           (snake[0].y + dy + GRID) % GRID };

            bool ate = (head.x == food.x && head.y == food.y);
            if (!ate) snake.pop_back();

            if (isOnSnake(snake, head)) {
                gameOver = true;
            } else {
                snake.insert(snake.begin(), head);
                if (ate) {
                    ++score;
                    food = spawnFood(snake);
                }
            }

            std::string title = "Snake - Score: " + std::to_string(score);
            if (gameOver) title += "  |  GAME OVER - press SPACE to restart";
            SDL_SetWindowTitle(window, title.c_str());
        }

        drawBackground(renderer);
        drawApple(renderer, food);
        drawSnake(renderer, snake, dx, dy, gameOver);
        SDL_RenderPresent(renderer);
        SDL_Delay(5);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}