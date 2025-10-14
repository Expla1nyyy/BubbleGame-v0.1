#include "raylib.h"
#include <vector>
#include <cmath>
#include <algorithm>
#include <random>

struct Ball {
    Vector2 position;
    Vector2 velocity;
    float radius;
    Color color;
    bool active;
    bool isStuck;

    Ball(float x, float y, float r, Color c)
        : position{ x, y }, velocity{ 0, 0 }, radius(r), color(c), active(true), isStuck(true) {}
};

class BallGame {
private:
    const int screenWidth = 800;
    const int screenHeight = 600;
    const float ballRadius = 12.0f;
    const float shootSpeed = 20.0f; // Увеличил скорость в 2 раза
    const float gravity = 0.05f;    // Уменьшил гравитацию для большего полета
    const float friction = 0.97f;
    const float minVelocity = 0.2f;

    std::vector<Ball> balls;
    Ball* currentBall;
    bool isAiming;
    Vector2 aimDirection;
    int score;
    bool gameOver;

    std::vector<Color> ballColors = {
        RED, BLUE, GREEN, YELLOW, PURPLE, ORANGE, PINK, SKYBLUE
    };

public:
    BallGame() : isAiming(false), score(0), gameOver(false), currentBall(nullptr) {
        InitWindow(screenWidth, screenHeight, "Шарики за ролики - МЕГА СИЛА!");
        SetTargetFPS(60);

        createInitialBalls();
        createNewBall();
    }

    ~BallGame() {
        if (currentBall) delete currentBall;
        CloseWindow();
    }

    void createInitialBalls() {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> colorDist(0, ballColors.size() - 1);

        // Создаем большой массив 7x20 шаров в ВЕРХНЕЙ части экрана
        for (int row = 0; row < 7; row++) {
            for (int col = 0; col < 20; col++) {
                float x = 20 + col * (ballRadius * 2 + 2);
                float y = 50 + row * (ballRadius * 2 + 2); // Еще выше
                if (x + ballRadius < screenWidth && y + ballRadius < screenHeight - 100) {
                    balls.emplace_back(x, y, ballRadius, ballColors[colorDist(gen)]);
                }
            }
        }

        // Добавляем еще случайных шаров для заполнения пробелов
        for (int i = 0; i < 30; i++) {
            float x = 30 + (rand() % (screenWidth - 60));
            float y = 50 + (rand() % 150); // Выше
            balls.emplace_back(x, y, ballRadius, ballColors[colorDist(gen)]);
        }
    }

    void createNewBall() {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> colorDist(0, ballColors.size() - 1);

        if (currentBall) {
            delete currentBall;
        }
        // Запускаем шар СИЛЬНО ВЫШЕ - почти из центра экрана
        currentBall = new Ball(screenWidth / 2, screenHeight - 250, ballRadius,
            ballColors[colorDist(gen)]);
        currentBall->isStuck = false;
        isAiming = true;
    }

    void update() {
        if (gameOver) return;

        if (isAiming) {
            handleAiming();
        }
        else {
            updatePhysics();
            checkCollisions();
            checkGameOver();
        }
    }

    void handleAiming() {
        Vector2 mousePos = GetMousePosition();
        aimDirection = {
            mousePos.x - currentBall->position.x,
            mousePos.y - currentBall->position.y
        };

        float length = sqrt(aimDirection.x * aimDirection.x + aimDirection.y * aimDirection.y);
        if (length > 0) {
            aimDirection.x /= length;
            aimDirection.y /= length;
        }

        // Увеличиваем максимальную силу выстрела
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            shootBall();
        }

        // Добавляем возможность усиленного выстрела при зажатии
        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
            // Чем дольше зажата кнопка, тем сильнее выстрел (до определенного предела)
            static float chargeTime = 0.0f;
            chargeTime += GetFrameTime();
            if (chargeTime > 1.5f) chargeTime = 1.5f;
        }
    }

    void shootBall() {
        // Базовая сила + дополнительная мощность
        float powerMultiplier = 1.0f;

        // Проверяем расстояние для определения силы
        float mouseDistance = sqrt(aimDirection.x * aimDirection.x + aimDirection.y * aimDirection.y);
        if (mouseDistance > 100.0f) {
            powerMultiplier = 1.5f; // Дополнительная сила при дальнем прицеливании
        }

        currentBall->velocity = {
            aimDirection.x * shootSpeed * powerMultiplier,
            aimDirection.y * shootSpeed * powerMultiplier
        };
        currentBall->isStuck = false;
        isAiming = false;
    }

    void updatePhysics() {
        // Обновляем физику для текущего шара
        if (currentBall && !currentBall->isStuck) {
            currentBall->velocity.y += gravity;
            currentBall->velocity.x *= friction;
            currentBall->velocity.y *= friction;

            currentBall->position.x += currentBall->velocity.x;
            currentBall->position.y += currentBall->velocity.y;

            // Столкновение со стенами
            if (currentBall->position.x - currentBall->radius < 0) {
                currentBall->position.x = currentBall->radius;
                currentBall->velocity.x *= -0.7f; // Меньше трения при отскоке
            }
            else if (currentBall->position.x + currentBall->radius > screenWidth) {
                currentBall->position.x = screenWidth - currentBall->radius;
                currentBall->velocity.x *= -0.7f;
            }

            if (currentBall->position.y - currentBall->radius < 30) { // Выше верхняя граница
                currentBall->position.y = 30 + currentBall->radius;
                currentBall->velocity.y *= -0.7f;
            }
        }

        // Обновляем незафиксированные шары в массиве
        for (auto& ball : balls) {
            if (!ball.isStuck && ball.active) {
                ball.velocity.y += gravity;
                ball.velocity.x *= friction;
                ball.velocity.y *= friction;

                ball.position.x += ball.velocity.x;
                ball.position.y += ball.velocity.y;

                // Столкновение со стенами
                if (ball.position.x - ball.radius < 0) {
                    ball.position.x = ball.radius;
                    ball.velocity.x *= -0.7f;
                }
                else if (ball.position.x + ball.radius > screenWidth) {
                    ball.position.x = screenWidth - ball.radius;
                    ball.velocity.x *= -0.7f;
                }

                if (ball.position.y - ball.radius < 30) {
                    ball.position.y = 30 + ball.radius;
                    ball.velocity.y *= -0.7f;
                }
            }
        }
    }

    void checkCollisions() {
        if (!currentBall || currentBall->isStuck) return;

        bool hasCollision = false;
        Ball* collidedBall = nullptr;

        // Проверяем столкновение с зафиксированными шарами
        for (auto& ball : balls) {
            if (!ball.active || !ball.isStuck) continue;

            float dx = currentBall->position.x - ball.position.x;
            float dy = currentBall->position.y - ball.position.y;
            float distance = sqrt(dx * dx + dy * dy);

            if (distance < currentBall->radius + ball.radius) {
                hasCollision = true;
                collidedBall = &ball;

                // Корректируем позицию
                float overlap = (currentBall->radius + ball.radius) - distance;
                if (distance > 0) {
                    currentBall->position.x += (dx / distance) * overlap * 0.5f;
                    currentBall->position.y += (dy / distance) * overlap * 0.5f;
                }
                break;
            }
        }

        // Более либеральные условия для фиксации - шар должен быть почти остановлен
        bool shouldStick = hasCollision ||
            (fabs(currentBall->velocity.x) < minVelocity &&
                fabs(currentBall->velocity.y) < minVelocity &&
                currentBall->position.y < screenHeight - 150);

        if (shouldStick) {
            currentBall->isStuck = true;
            currentBall->velocity = { 0, 0 };

            // Находим ближайшую позицию для прилипания
            if (hasCollision && collidedBall) {
                float dx = currentBall->position.x - collidedBall->position.x;
                float dy = currentBall->position.y - collidedBall->position.y;
                float distance = sqrt(dx * dx + dy * dy);

                if (distance > 0) {
                    float targetDistance = currentBall->radius + collidedBall->radius;
                    currentBall->position.x = collidedBall->position.x + (dx / distance) * targetDistance;
                    currentBall->position.y = collidedBall->position.y + (dy / distance) * targetDistance;
                }
            }

            balls.push_back(*currentBall);

            // Проверяем группы
            checkBallGroups();

            createNewBall();
        }

        // Если шар улетел за нижнюю границу, создаем новый (реже происходит)
        if (currentBall && currentBall->position.y > screenHeight - 20) {
            createNewBall();
        }
    }

    void checkBallGroups() {
        if (balls.empty()) return;

        std::vector<int> toRemove;
        std::vector<bool> visited(balls.size(), false);

        for (size_t i = 0; i < balls.size(); i++) {
            if (!balls[i].active || visited[i] || !balls[i].isStuck) continue;

            std::vector<int> group;
            findConnectedBalls(i, group, balls[i].color, visited);

            if (group.size() >= 3) {
                toRemove.insert(toRemove.end(), group.begin(), group.end());
                score += group.size() * 15; // Больше очков

                if (group.size() >= 5) {
                    score += group.size() * 10;
                }
                if (group.size() >= 7) {
                    score += group.size() * 20;
                }
                if (group.size() >= 10) {
                    score += group.size() * 30; // Мега бонус за огромные группы
                }
            }
        }

        // Удаляем шары из групп
        for (int index : toRemove) {
            balls[index].active = false;
        }

        // Очищаем неактивные шары
        if (!toRemove.empty()) {
            balls.erase(std::remove_if(balls.begin(), balls.end(),
                [](const Ball& ball) { return !ball.active; }),
                balls.end());
        }
    }

    void findConnectedBalls(int startIndex, std::vector<int>& group, Color targetColor, std::vector<bool>& visited) {
        if (visited[startIndex]) return;

        visited[startIndex] = true;
        group.push_back(startIndex);

        for (size_t i = 0; i < balls.size(); i++) {
            if (visited[i] || !balls[i].active || !balls[i].isStuck) continue;

            if (colorsEqual(balls[i].color, targetColor)) {
                float dx = balls[i].position.x - balls[startIndex].position.x;
                float dy = balls[i].position.y - balls[startIndex].position.y;
                float distance = sqrt(dx * dx + dy * dy);

                if (distance < ballRadius * 2.2f) {
                    findConnectedBalls(i, group, targetColor, visited);
                }
            }
        }
    }

    bool colorsEqual(Color a, Color b) {
        return a.r == b.r && a.g == b.g && a.b == b.b;
    }

    void checkGameOver() {
        // Game over только при ОЧЕНЬ большом количестве шаров
        if (balls.size() > 250) {
            gameOver = true;
        }

        // Дополнительно: если шары опустились слишком низко
        float lowestBall = 0;
        for (const auto& ball : balls) {
            if (ball.isStuck && ball.active && ball.position.y > lowestBall) {
                lowestBall = ball.position.y;
            }
        }

        if (lowestBall > screenHeight - 50) {
            gameOver = true;
        }
    }

    void draw() {
        BeginDrawing();
        ClearBackground(BLACK);

        // Рисуем фон
        DrawRectangle(0, screenHeight - 50, screenWidth, 50, DARKGRAY);
        DrawRectangle(0, 0, screenWidth, 50, DARKGRAY);

        // Рисуем границы игровой зоны
        DrawRectangleLines(10, 50, screenWidth - 20, screenHeight - 100, GRAY);

        // Рисуем все шары
        for (const auto& ball : balls) {
            if (ball.active) {
                DrawCircleV(ball.position, ball.radius, ball.color);
                DrawCircleLines(ball.position.x, ball.position.y, ball.radius,
                    ball.isStuck ? Fade(WHITE, 0.5f) : Fade(GRAY, 0.5f));
            }
        }

        // Рисуем текущий шар и прицел
        if (currentBall) {
            DrawCircleV(currentBall->position, currentBall->radius, currentBall->color);
            DrawCircleLines(currentBall->position.x, currentBall->position.y,
                currentBall->radius, YELLOW);

            if (isAiming) {
                Vector2 endPoint = {
                    currentBall->position.x + aimDirection.x * 150, // Удлиненный прицел
                    currentBall->position.y + aimDirection.y * 150
                };
                DrawLineV(currentBall->position, endPoint, YELLOW);
                DrawCircleV(endPoint, 5, RED);

                // Показываем силу выстрела
                float power = sqrt(aimDirection.x * aimDirection.x + aimDirection.y * aimDirection.y) * 80;
                DrawRectangle(screenWidth - 150, screenHeight - 45, (int)power, 10,
                    power > 60 ? RED : (power > 40 ? ORANGE : GREEN));
            }
        }

        // Рисуем UI
        DrawText(TextFormat("Счет: %d", score), 20, 15, 20, WHITE);
        DrawText(TextFormat("Шаров: %zu", balls.size()), screenWidth - 150, 15, 20, WHITE);
        DrawText("МЕГА СИЛА! Стреляйте сильно!", screenWidth / 2 - 140, 15, 20, YELLOW);

        // Подсказки по управлению
        DrawText("ЛКМ - выстрел, R - перезапуск", 20, screenHeight - 30, 15, LIGHTGRAY);
        DrawText("Сила выстрела:", screenWidth - 180, screenHeight - 45, 15, GREEN);

        // Отображаем мощность
        if (isAiming) {
            float power = sqrt(aimDirection.x * aimDirection.x + aimDirection.y * aimDirection.y) * 100;
            DrawText(TextFormat("Мощность: %.0f%%", power), screenWidth / 2 - 60, screenHeight - 100, 18,
                power > 80 ? RED : (power > 50 ? YELLOW : GREEN));
        }

        if (gameOver) {
            DrawRectangle(0, 0, screenWidth, screenHeight, Fade(BLACK, 0.8f));
            DrawText("ИГРА ОКОНЧЕНА!", screenWidth / 2 - 120, screenHeight / 2 - 60, 40, RED);
            DrawText(TextFormat("Финальный счет: %d", score), screenWidth / 2 - 100, screenHeight / 2 - 10, 30, WHITE);
            DrawText(TextFormat("Уничтожено групп: %d", score / 15), screenWidth / 2 - 120, screenHeight / 2 + 30, 25, YELLOW);
            DrawText("Нажмите R для новой игры", screenWidth / 2 - 140, screenHeight / 2 + 80, 25, GREEN);
        }

        EndDrawing();
    }

    void run() {
        while (!WindowShouldClose()) {
            if (gameOver && IsKeyPressed(KEY_R)) {
                restart();
            }

            update();
            draw();
        }
    }

    void restart() {
        balls.clear();
        if (currentBall) {
            delete currentBall;
            currentBall = nullptr;
        }
        score = 0;
        gameOver = false;
        createInitialBalls();
        createNewBall();
    }
};

int main() {
    BallGame game;
    game.run();
    return 0;
}