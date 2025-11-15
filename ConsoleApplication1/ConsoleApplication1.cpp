#include "raylib.h"
#include <vector>
#include <cmath>
#include <algorithm>
#include <random>
#include <unordered_set>

// Добавляем enum для состояний игры
enum GameState {
    MAIN_MENU,
    PLAYING,
    GAME_OVER,
    GAME_WON
};

struct Ball {
    Vector2 position;
    Vector2 velocity;
    Vector2 acceleration;
    float radius;
    Color color;
    bool active;
    bool isStuck;
    float stiffness;
    float damping;
    Vector2 originalPosition;
    bool hasSupport; // Новое поле: имеет ли шар опору сверху

    Ball(float x, float y, float r, Color c)
        : position{ x, y }, velocity{ 0, 0 }, acceleration{ 0, 0 },
        radius(r), color(c), active(true), isStuck(true),
        stiffness(0.08f), damping(0.92f), originalPosition{ x, y }, hasSupport(true) {
    }
};

class BallGame {
private:
    const int screenWidth = 450;
    const int screenHeight = 800;
    const float ballRadius = 15.0f;
    const float shootSpeed = 17.0f;
    const float minVelocity = 0.1f;

    const float gameAreaLeft = 10.0f;
    const float gameAreaTop = 60.0f;
    const float gameAreaRight = 440.0f;
    const float gameAreaBottom = 750.0f;
    const float gameAreaWidth = 430.0f;
    const float gameAreaHeight = 690.0f;

    const float connectionStrength = 0.05f;
    const float magnetStrength = 0.3f;
    const float maxMagnetDistance = 60.0f;
    const float separationForce = 0.1f;
    const float maxBallSpeed = 2.0f;
    const float antiGravity = -0.2f; // Отрицательная гравитация - поднимает шары вверх
    const float clusterMagnetStrength = 2.0f; // УВЕЛИЧЕНА: Сила притяжения к кластеру
    const float maxClusterMagnetDistance = 300.0f; // УВЕЛИЧЕНА: Максимальная дистанция притяжения к кластеру

    std::vector<Ball> balls;
    Ball* currentBall;
    bool isAiming;
    Vector2 aimDirection;
    int score;
    GameState gameState; // Изменяем на enum

    Vector2 newBallPosition;

    std::vector<Color> ballColors = {
        RED, BLUE, GREEN, YELLOW, PURPLE, ORANGE, PINK, SKYBLUE, LIME, VIOLET
    };

    // Кнопки меню
    Rectangle startButton;
    Rectangle exitButton;

public:
    BallGame() : isAiming(false), score(0), gameState(MAIN_MENU), currentBall(nullptr) {
        InitWindow(screenWidth, screenHeight, "BubbleBlast");
        SetTargetFPS(60);

        newBallPosition = { static_cast<float>(screenWidth) / 2.0f, gameAreaBottom - 30.0f };

        // Инициализация кнопок меню
        startButton = { screenWidth / 2.0f - 100.0f, screenHeight / 2.0f, 200.0f, 50.0f };
        exitButton = { screenWidth / 2.0f - 100.0f, screenHeight / 2.0f + 70.0f, 200.0f, 50.0f };

        createInitialBalls();
        createNewBall();
    }

    ~BallGame() {
        if (currentBall) delete currentBall;
        CloseWindow();
    }

    void createInitialBalls() {
        int ballsPerRow = static_cast<int>(gameAreaWidth / (ballRadius * 2.0f));
        int rows = 10;

        std::vector<std::vector<Color>> colorGrid(static_cast<size_t>(rows),
            std::vector<Color>(static_cast<size_t>(ballsPerRow), BLACK));

        for (int row = 0; row < rows; row++) {
            for (int col = 0; col < ballsPerRow; col++) {
                colorGrid[static_cast<size_t>(row)][static_cast<size_t>(col)] = getColorForPosition(colorGrid, row, col);
            }
        }

        float totalWidth = static_cast<float>(ballsPerRow) * ballRadius * 2.0f;
        float startX = gameAreaLeft + (gameAreaWidth - totalWidth) / 2.0f + ballRadius;

        for (int row = 0; row < rows; row++) {
            for (int col = 0; col < ballsPerRow; col++) {
                float x = startX + static_cast<float>(col) * (ballRadius * 2.0f);
                float y = gameAreaTop + 10.0f + static_cast<float>(row) * (ballRadius * 2.0f);

                if (x + ballRadius < gameAreaRight && y + ballRadius < gameAreaBottom) {
                    balls.emplace_back(x, y, ballRadius, colorGrid[static_cast<size_t>(row)][static_cast<size_t>(col)]);
                    // Шары в верхнем ряду всегда имеют опору
                    balls.back().hasSupport = (row == 0);
                }
            }
        }
    }

    Color getColorForPosition(std::vector<std::vector<Color>>& grid, int row, int col) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> colorDist(0, static_cast<int>(ballColors.size()) - 1);

        for (int attempt = 0; attempt < 50; attempt++) {
            Color candidate = ballColors[static_cast<size_t>(colorDist(gen))];

            if (isColorSafe(grid, row, col, candidate)) {
                return candidate;
            }
        }

        return getFallbackColor(grid, row, col);
    }

    bool isColorSafe(std::vector<std::vector<Color>>& grid, int row, int col, Color color) {
        if (col >= 2) {
            Color left1 = grid[static_cast<size_t>(row)][static_cast<size_t>(col - 1)];
            Color left2 = grid[static_cast<size_t>(row)][static_cast<size_t>(col - 2)];
            if (colorsEqual(color, left1) && colorsEqual(color, left2)) {
                return false;
            }
        }

        if (row >= 2) {
            Color above1 = grid[static_cast<size_t>(row - 1)][static_cast<size_t>(col)];
            Color above2 = grid[static_cast<size_t>(row - 2)][static_cast<size_t>(col)];
            if (colorsEqual(color, above1) && colorsEqual(color, above2)) {
                return false;
            }
        }

        return true;
    }

    Color getFallbackColor(std::vector<std::vector<Color>>& grid, int row, int col) {
        std::random_device rd;
        std::mt19937 gen(rd());

        for (size_t i = 0; i < ballColors.size(); i++) {
            Color candidate = ballColors[i];
            bool safeFromImmediate = true;

            if (col >= 1 && colorsEqual(candidate, grid[static_cast<size_t>(row)][static_cast<size_t>(col - 1)])) {
                safeFromImmediate = false;
            }

            if (row >= 1 && colorsEqual(candidate, grid[static_cast<size_t>(row - 1)][static_cast<size_t>(col)])) {
                safeFromImmediate = false;
            }

            if (safeFromImmediate) {
                return candidate;
            }
        }

        std::uniform_int_distribution<> colorDist(0, static_cast<int>(ballColors.size()) - 1);
        return ballColors[static_cast<size_t>(colorDist(gen))];
    }

    void createNewBall() {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> colorDist(0, static_cast<int>(ballColors.size()) - 1);

        if (currentBall) {
            delete currentBall;
        }

        currentBall = new Ball(newBallPosition.x, newBallPosition.y, ballRadius,
            ballColors[static_cast<size_t>(colorDist(gen))]);
        currentBall->isStuck = false;
        currentBall->originalPosition = newBallPosition;
        currentBall->hasSupport = true; // Новый шар имеет опору (находится внизу)
        isAiming = true;
    }

    void update() {
        if (gameState == MAIN_MENU) {
            updateMainMenu();
        }
        else if (gameState == PLAYING) {
            updateGame();
        }
    }

    void updateMainMenu() {
        Vector2 mousePoint = GetMousePosition();

        // Проверка нажатия на кнопки
        if (CheckCollisionPointRec(mousePoint, startButton)) {
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                gameState = PLAYING;
            }
        }

        if (CheckCollisionPointRec(mousePoint, exitButton)) {
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                CloseWindow();
            }
        }
    }

    void updateGame() {
        if (isAiming) {
            handleAiming();
            updateBallPhysics();
        }
        else {
            updatePhysics();
            checkCollisions();
            updateBallPhysics();
            checkSupport(); // Проверяем опору для всех шаров
            applyClusterMagnetForces(); // ПЕРЕМЕЩЕНА ВЫШЕ: Применяем притяжение к кластеру ДО антигравитации
            applyAntiGravity(); // Применяем антигравитацию к шарам без опоры
            checkGameOver();
            checkGameWin();
        }
    }

    void handleAiming() {
        Vector2 mousePos = GetMousePosition();

        Vector2 targetPosition = {
            mousePos.x,
            mousePos.y
        };

        float maxAimDistance = 100.0f;
        float dx = targetPosition.x - newBallPosition.x;
        float dy = targetPosition.y - newBallPosition.y;
        float distance = sqrtf(dx * dx + dy * dy);

        if (distance > maxAimDistance) {
            targetPosition.x = newBallPosition.x + (dx / distance) * maxAimDistance;
            targetPosition.y = newBallPosition.y + (dy / distance) * maxAimDistance;
        }

        if (targetPosition.x - ballRadius < gameAreaLeft) {
            targetPosition.x = gameAreaLeft + ballRadius;
        }
        else if (targetPosition.x + ballRadius > gameAreaRight) {
            targetPosition.x = gameAreaRight - ballRadius;
        }

        // НЕ позволяем целиться за нижний край экрана
        if (targetPosition.y - ballRadius < gameAreaTop) {
            targetPosition.y = gameAreaTop + ballRadius;
        }
        else if (targetPosition.y + ballRadius > gameAreaBottom) {
            targetPosition.y = gameAreaBottom - ballRadius;
        }

        // Дополнительная проверка: не позволяем целиться ниже стартовой позиции
        if (targetPosition.y > newBallPosition.y) {
            targetPosition.y = newBallPosition.y;
        }

        float smoothSpeed = 0.3f;
        currentBall->position.x += (targetPosition.x - currentBall->position.x) * smoothSpeed;
        currentBall->position.y += (targetPosition.y - currentBall->position.y) * smoothSpeed;

        aimDirection = {
            currentBall->position.x - newBallPosition.x,
            currentBall->position.y - newBallPosition.y
        };

        float length = sqrtf(aimDirection.x * aimDirection.x + aimDirection.y * aimDirection.y);
        if (length > 0.0f) {
            aimDirection.x /= length;
            aimDirection.y /= length;
        }

        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            shootBall();
        }
    }

    void shootBall() {
        float dx = currentBall->position.x - newBallPosition.x;
        float dy = currentBall->position.y - newBallPosition.y;
        float distance = sqrtf(dx * dx + dy * dy);
        float power = distance / 50.0f;

        if (power > 1.5f) power = 1.5f;
        if (power < 0.3f) power = 0.3f;

        currentBall->velocity = {
            aimDirection.x * shootSpeed * power,
            aimDirection.y * shootSpeed * power
        };
        currentBall->isStuck = false;
        currentBall->hasSupport = false; // Выстреленный шар временно без опоры
        isAiming = false;
    }

    void updatePhysics() {
        if (currentBall && !currentBall->isStuck) {
            float currentSpeed = sqrtf(currentBall->velocity.x * currentBall->velocity.x +
                currentBall->velocity.y * currentBall->velocity.y);
            if (currentSpeed < 8.0f) {
                applyMagnetForces(*currentBall);
            }

            currentBall->position.x += currentBall->velocity.x;
            currentBall->position.y += currentBall->velocity.y;

            if (currentBall->position.x - currentBall->radius < gameAreaLeft) {
                currentBall->position.x = gameAreaLeft + currentBall->radius;
                currentBall->velocity.x *= -0.7f;
            }
            else if (currentBall->position.x + currentBall->radius > gameAreaRight) {
                currentBall->position.x = gameAreaRight - currentBall->radius;
                currentBall->velocity.x *= -0.7f;
            }

            if (currentBall->position.y - currentBall->radius < gameAreaTop) {
                currentBall->position.y = gameAreaTop + currentBall->radius;
                currentBall->velocity.y *= -0.7f;
            }

            // НЕ позволяем шару улететь за нижний край при выстреле
            if (currentBall->position.y + currentBall->radius > gameAreaBottom) {
                currentBall->position.y = gameAreaBottom - currentBall->radius;
                currentBall->velocity.y *= -0.7f;
            }

            currentBall->velocity.x *= 0.99f;
            currentBall->velocity.y *= 0.99f;

            if (fabsf(currentBall->velocity.x) < minVelocity) currentBall->velocity.x = 0.0f;
            if (fabsf(currentBall->velocity.y) < minVelocity) currentBall->velocity.y = 0.0f;
        }
    }

    void applyMagnetForces(Ball& movingBall) {
        for (auto& ball : balls) {
            if (!ball.active || !ball.isStuck) continue;

            float dx = ball.position.x - movingBall.position.x;
            float dy = ball.position.y - movingBall.position.y;
            float distance = sqrtf(dx * dx + dy * dy);

            if (distance < maxMagnetDistance && distance > ballRadius * 2.5f) {
                float force = magnetStrength * (1.0f - distance / maxMagnetDistance);
                force *= 0.3f;

                float forceX = (dx / distance) * force;
                float forceY = (dy / distance) * force;

                movingBall.velocity.x += forceX;
                movingBall.velocity.y += forceY;
            }
        }
    }

    // УЛУЧШЕННАЯ функция: притяжение шаров без опоры к основному кластеру
    void applyClusterMagnetForces() {
        // Находим центр масс основного кластера (шаров с опорой)
        Vector2 clusterCenter = { 0.0f, 0.0f };
        int clusterCount = 0;

        for (auto& ball : balls) {
            if (!ball.active || !ball.isStuck || !ball.hasSupport) continue;

            clusterCenter.x += ball.position.x;
            clusterCenter.y += ball.position.y;
            clusterCount++;
        }

        // Если нет основного кластера, используем нижнюю часть экрана как точку притяжения
        if (clusterCount == 0) {
            clusterCenter = { screenWidth / 2.0f, gameAreaBottom - 100.0f };
            clusterCount = 1;
        }
        else {
            clusterCenter.x /= clusterCount;
            clusterCenter.y /= clusterCount;
        }

        // Применяем притяжение к шарикам без опоры
        for (auto& ball : balls) {
            if (!ball.active || !ball.isStuck || ball.hasSupport) continue;

            float dx = clusterCenter.x - ball.position.x;
            float dy = clusterCenter.y - ball.position.y;
            float distance = sqrtf(dx * dx + dy * dy);

            // УСИЛЕННОЕ притяжение - работает на любой дистанции и сильнее
            if (distance > ballRadius * 2.0f) {
                float force = clusterMagnetStrength * (0.5f + distance / 100.0f);

                // Увеличиваем силу для далеких шаров
                if (distance > 100.0f) force *= 2.0f;

                float forceX = (dx / distance) * force;
                float forceY = (dy / distance) * force;

                ball.velocity.x += forceX;
                ball.velocity.y += forceY;

                // Ограничиваем скорость притяжения
                float speed = sqrtf(ball.velocity.x * ball.velocity.x + ball.velocity.y * ball.velocity.y);
                if (speed > maxBallSpeed * 3.0f) {
                    ball.velocity.x = (ball.velocity.x / speed) * maxBallSpeed * 3.0f;
                    ball.velocity.y = (ball.velocity.y / speed) * maxBallSpeed * 3.0f;
                }
            }
        }

        // Также притягиваем текущий шар к кластеру, если он летит медленно
        if (currentBall && !currentBall->isStuck) {
            float currentSpeed = sqrtf(currentBall->velocity.x * currentBall->velocity.x +
                currentBall->velocity.y * currentBall->velocity.y);
            if (currentSpeed < 10.0f) {
                float dx = clusterCenter.x - currentBall->position.x;
                float dy = clusterCenter.y - currentBall->position.y;
                float distance = sqrtf(dx * dx + dy * dy);

                if (distance > ballRadius * 3.0f) {
                    float force = clusterMagnetStrength * 0.7f * (0.5f + distance / 100.0f);

                    float forceX = (dx / distance) * force;
                    float forceY = (dy / distance) * force;

                    currentBall->velocity.x += forceX;
                    currentBall->velocity.y += forceY;
                }
            }
        }
    }

    // Новая функция: проверка опоры для шаров (сверху)
    void checkSupport() {
        // Сначала сбрасываем флаги опоры для всех шаров
        for (auto& ball : balls) {
            if (!ball.active || !ball.isStuck) continue;
            ball.hasSupport = false;
        }

        // Шары на верху игровой области всегда имеют опору
        for (auto& ball : balls) {
            if (!ball.active || !ball.isStuck) continue;
            if (ball.position.y - ball.radius <= gameAreaTop + 1.0f) {
                ball.hasSupport = true;
            }
        }

        // Распространяем опору сверху вниз
        bool changed;
        do {
            changed = false;
            for (auto& ball : balls) {
                if (!ball.active || !ball.isStuck || ball.hasSupport) continue;

                // Проверяем, есть ли над этим шаром шар с опорой
                for (const auto& other : balls) {
                    if (!other.active || !other.isStuck || !other.hasSupport) continue;

                    // Проверяем, находится ли другой шар достаточно близко сверху
                    float dx = other.position.x - ball.position.x;
                    float dy = other.position.y - ball.position.y;
                    float distance = sqrtf(dx * dx + dy * dy);

                    // Учитываем шары сверху (с небольшим допуском по вертикали)
                    if (distance < ballRadius * 2.2f && other.position.y < ball.position.y + ballRadius) {
                        ball.hasSupport = true;
                        changed = true;
                        break;
                    }
                }
            }
        } while (changed); // Повторяем, пока находятся новые шары с опорой
    }

    // Новая функция: применение антигравитации к шарам без опоры
    void applyAntiGravity() {
        for (auto& ball : balls) {
            if (!ball.active || !ball.isStuck || ball.hasSupport) continue;

            // УМЕНЬШЕНА антигравитация, чтобы не мешать притяжению
            ball.velocity.y += antiGravity * 0.3f;

            // Ограничиваем максимальную скорость подъема
            if (ball.velocity.y < -1.5f) {
                ball.velocity.y = -1.5f;
            }
        }
    }

    void updateBallPhysics() {
        resolveOverlaps();
        updateConnections();
        applyDampingAndLimits();
    }

    void resolveOverlaps() {
        for (size_t i = 0; i < balls.size(); i++) {
            if (!balls[i].active || !balls[i].isStuck) continue;

            for (size_t j = i + 1; j < balls.size(); j++) {
                if (!balls[j].active || !balls[j].isStuck) continue;

                float dx = balls[j].position.x - balls[i].position.x;
                float dy = balls[j].position.y - balls[i].position.y;
                float distance = sqrtf(dx * dx + dy * dy);
                float minDistance = balls[i].radius + balls[j].radius;

                if (distance < minDistance && distance > 0.1f) {
                    float overlap = (minDistance - distance) * 0.5f;
                    float moveX = (dx / distance) * overlap * separationForce;
                    float moveY = (dy / distance) * overlap * separationForce;

                    balls[i].position.x -= moveX;
                    balls[i].position.y -= moveY;
                    balls[j].position.x += moveX;
                    balls[j].position.y += moveY;
                }
            }
        }
    }

    void updateConnections() {
        for (size_t i = 0; i < balls.size(); i++) {
            if (!balls[i].active || !balls[i].isStuck) continue;

            Vector2 totalForce = { 0.0f, 0.0f };
            int connectionCount = 0;

            for (size_t j = 0; j < balls.size(); j++) {
                if (i == j || !balls[j].active || !balls[j].isStuck) continue;

                float dx = balls[j].position.x - balls[i].position.x;
                float dy = balls[j].position.y - balls[i].position.y;
                float distance = sqrtf(dx * dx + dy * dy);

                if (distance < ballRadius * 2.8f) {
                    float targetDistance = ballRadius * 2.0f;
                    float displacement = distance - targetDistance;

                    if (fabsf(displacement) > 0.5f) {
                        float force = displacement * balls[i].stiffness;
                        if (distance > ballRadius * 2.2f) {
                            force *= 0.3f;
                        }

                        totalForce.x += (dx / distance) * force;
                        totalForce.y += (dy / distance) * force;
                        connectionCount++;
                    }
                }
            }

            float restoreForce = 0.01f;
            totalForce.x += (balls[i].originalPosition.x - balls[i].position.x) * restoreForce;
            totalForce.y += (balls[i].originalPosition.y - balls[i].position.y) * restoreForce;

            if (connectionCount > 0 || restoreForce > 0.0f) {
                balls[i].velocity.x += totalForce.x;
                balls[i].velocity.y += totalForce.y;
            }
        }
    }

    void applyDampingAndLimits() {
        for (auto& ball : balls) {
            if (!ball.active || !ball.isStuck) continue;

            ball.velocity.x *= ball.damping;
            ball.velocity.y *= ball.damping;

            float speed = sqrtf(ball.velocity.x * ball.velocity.x + ball.velocity.y * ball.velocity.y);
            if (speed > maxBallSpeed) {
                ball.velocity.x = (ball.velocity.x / speed) * maxBallSpeed;
                ball.velocity.y = (ball.velocity.y / speed) * maxBallSpeed;
            }

            if (fabsf(ball.velocity.x) < 0.05f) ball.velocity.x = 0.0f;
            if (fabsf(ball.velocity.y) < 0.05f) ball.velocity.y = 0.0f;

            ball.position.x += ball.velocity.x;
            ball.position.y += ball.velocity.y;

            const float margin = 5.0f;
            if (ball.position.x - ball.radius < gameAreaLeft + margin) {
                ball.position.x = gameAreaLeft + ball.radius + margin;
                ball.velocity.x = 0.0f;
            }
            else if (ball.position.x + ball.radius > gameAreaRight - margin) {
                ball.position.x = gameAreaRight - ball.radius - margin;
                ball.velocity.x = 0.0f;
            }

            // Обработка достижения верха
            if (ball.position.y - ball.radius < gameAreaTop) {
                ball.position.y = gameAreaTop + ball.radius;
                ball.velocity.y = 0.0f;
                ball.hasSupport = true; // Шар на верху имеет опору
            }

            // Обработка достижения низа
            if (ball.position.y + ball.radius > gameAreaBottom) {
                ball.position.y = gameAreaBottom - ball.radius;
                ball.velocity.y = 0.0f;
            }
        }
    }

    void checkCollisions() {
        if (!currentBall || currentBall->isStuck) return;

        bool hasCollision = false;
        Ball* closestBall = nullptr;
        float minDistance = std::numeric_limits<float>::max();

        for (auto& ball : balls) {
            if (!ball.active || !ball.isStuck) continue;

            float dx = currentBall->position.x - ball.position.x;
            float dy = currentBall->position.y - ball.position.y;
            float distance = sqrtf(dx * dx + dy * dy);

            if (distance < currentBall->radius + ball.radius) {
                if (distance < minDistance) {
                    minDistance = distance;
                    hasCollision = true;
                    closestBall = &ball;
                }
            }
        }

        if (hasCollision && closestBall) {
            currentBall->isStuck = true;
            currentBall->hasSupport = closestBall->hasSupport; // Наследуем опору

            float impactTransfer = 0.1f;
            closestBall->velocity.x += currentBall->velocity.x * impactTransfer;
            closestBall->velocity.y += currentBall->velocity.y * impactTransfer;

            float dx = currentBall->position.x - closestBall->position.x;
            float dy = currentBall->position.y - closestBall->position.y;
            float distance = sqrtf(dx * dx + dy * dy);
            float targetDistance = currentBall->radius + closestBall->radius;

            if (distance > 0.0f) {
                currentBall->position.x = closestBall->position.x + (dx / distance) * targetDistance;
                currentBall->position.y = closestBall->position.y + (dy / distance) * targetDistance;
                currentBall->originalPosition = currentBall->position;
            }

            balls.push_back(*currentBall);
            checkBallGroups();
            createNewBall();
        }

        if (currentBall && (currentBall->position.y > gameAreaBottom + 50.0f ||
            currentBall->position.y < gameAreaTop - 50.0f ||
            currentBall->position.x < gameAreaLeft - 50.0f ||
            currentBall->position.x > gameAreaRight + 50.0f)) {
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
            findConnectedBalls(static_cast<int>(i), group, balls[i].color, visited);

            if (group.size() >= 3) {
                toRemove.insert(toRemove.end(), group.begin(), group.end());
                score += static_cast<int>(group.size()) * 15;

                if (group.size() >= 5) score += static_cast<int>(group.size()) * 10;
                if (group.size() >= 7) score += static_cast<int>(group.size()) * 20;
                if (group.size() >= 10) score += static_cast<int>(group.size()) * 30;
            }
        }

        for (int index : toRemove) {
            balls[static_cast<size_t>(index)].active = false;
        }

        if (!toRemove.empty()) {
            balls.erase(std::remove_if(balls.begin(), balls.end(),
                [](const Ball& ball) { return !ball.active; }),
                balls.end());

            applyGentleRemovalImpulse();
        }
    }

    void applyGentleRemovalImpulse() {
        for (auto& ball : balls) {
            if (ball.isStuck) {
                int randomX = GetRandomValue(-5, 5);
                int randomY = GetRandomValue(-5, 5);
                ball.velocity.x += static_cast<float>(randomX) / 100.0f;
                ball.velocity.y += static_cast<float>(randomY) / 100.0f;
            }
        }
    }

    void findConnectedBalls(int startIndex, std::vector<int>& group, Color targetColor, std::vector<bool>& visited) {
        if (visited[static_cast<size_t>(startIndex)]) return;

        visited[static_cast<size_t>(startIndex)] = true;
        group.push_back(startIndex);

        for (size_t i = 0; i < balls.size(); i++) {
            if (visited[i] || !balls[i].active || !balls[i].isStuck) continue;

            if (colorsEqual(balls[i].color, targetColor)) {
                float dx = balls[i].position.x - balls[static_cast<size_t>(startIndex)].position.x;
                float dy = balls[i].position.y - balls[static_cast<size_t>(startIndex)].position.y;
                float distance = sqrtf(dx * dx + dy * dy);

                if (distance < ballRadius * 2.2f) {
                    findConnectedBalls(static_cast<int>(i), group, targetColor, visited);
                }
            }
        }
    }

    bool colorsEqual(Color a, Color b) {
        return a.r == b.r && a.g == b.g && a.b == b.b;
    }

    void checkGameOver() {
        if (balls.size() > 175) {
            gameState = GAME_OVER;
        }
    }

    void checkGameWin() {
        if (balls.empty()) {
            gameState = GAME_WON;
        }
    }

    void draw() {
        BeginDrawing();
        ClearBackground(BLACK);

        if (gameState == MAIN_MENU) {
            drawMainMenu();
        }
        else if (gameState == PLAYING) {
            drawGame();
        }
        else if (gameState == GAME_OVER || gameState == GAME_WON) {
            drawGame();
            drawEndScreen();
        }

        EndDrawing();
    }

    void drawMainMenu() {
        // Фон меню
        DrawRectangleGradientV(0, 0, screenWidth, screenHeight, DARKBLUE, BLACK);

        // Название игры
        DrawText("BubbleBlast", screenWidth / 2 - MeasureText("BubbleBlast", 50) / 2, 150, 50, WHITE);

        // Кнопка начала игры
        Color startColor = LIGHTGRAY;
        if (CheckCollisionPointRec(GetMousePosition(), startButton)) {
            startColor = GREEN;
        }
        DrawRectangleRec(startButton, startColor);
        DrawRectangleLinesEx(startButton, 2, DARKGRAY);
        DrawText("Start Game",
            startButton.x + startButton.width / 2 - MeasureText("Start Game", 20) / 2,
            startButton.y + startButton.height / 2 - 10,
            20, DARKBLUE);

        // Кнопка выхода
        Color exitColor = LIGHTGRAY;
        if (CheckCollisionPointRec(GetMousePosition(), exitButton)) {
            exitColor = RED;
        }
        DrawRectangleRec(exitButton, exitColor);
        DrawRectangleLinesEx(exitButton, 2, DARKGRAY);
        DrawText("Exit",
            exitButton.x + exitButton.width / 2 - MeasureText("Exit", 20) / 2,
            exitButton.y + exitButton.height / 2 - 10,
            20, DARKBLUE);
    }

    void drawGame() {
        DrawRectangle(0, screenHeight - 50, screenWidth, 50, DARKGRAY);
        DrawRectangle(0, 0, screenWidth, 60, DARKGRAY);

        DrawRectangle(static_cast<int>(gameAreaLeft), static_cast<int>(gameAreaTop),
            static_cast<int>(gameAreaWidth), static_cast<int>(gameAreaHeight), Fade(DARKBLUE, 0.1f));
        DrawRectangleLines(static_cast<int>(gameAreaLeft), static_cast<int>(gameAreaTop),
            static_cast<int>(gameAreaWidth), static_cast<int>(gameAreaHeight), BLUE);

        DrawCircleLines(static_cast<int>(newBallPosition.x), static_cast<int>(newBallPosition.y),
            static_cast<int>(ballRadius), Fade(GREEN, 0.3f));

        drawMinimalConnections();

        for (const auto& ball : balls) {
            if (ball.active) {
                DrawCircleV(ball.position, ball.radius, ball.color);
                DrawCircleLines(static_cast<int>(ball.position.x), static_cast<int>(ball.position.y),
                    static_cast<int>(ball.radius), Fade(WHITE, 0.3f));
            }
        }

        if (currentBall) {
            DrawCircleV(currentBall->position, ballRadius, currentBall->color);
            DrawCircleLines(static_cast<int>(currentBall->position.x), static_cast<int>(currentBall->position.y),
                static_cast<int>(ballRadius), YELLOW);

            if (isAiming) {
                Vector2 endPoint = {
                    currentBall->position.x + aimDirection.x * 200.0f,
                    currentBall->position.y + aimDirection.y * 200.0f
                };
                DrawLineV(currentBall->position, endPoint, Fade(YELLOW, 0.7f));
                DrawCircleV(endPoint, 3.0f, RED);

                float power = sqrtf(
                    (currentBall->position.x - newBallPosition.x) * (currentBall->position.x - newBallPosition.x) +
                    (currentBall->position.y - newBallPosition.y) * (currentBall->position.y - newBallPosition.y)
                ) / 50.0f;

                if (power > 1.5f) power = 1.5f;
                DrawText(TextFormat("Power: %.1f", power),
                    static_cast<int>(currentBall->position.x - 30.0f),
                    static_cast<int>(currentBall->position.y - 40.0f),
                    12, WHITE);
            }
        }

        DrawText(TextFormat("Score: %d", score), 20, 20, 20, WHITE);
        DrawText(TextFormat("Balls: %zu", balls.size()), screenWidth - 120, 20, 20, WHITE);

        DrawText("LMB - shoot, R - restart", 20, screenHeight - 30, 15, LIGHTGRAY);
    }

    void drawEndScreen() {
        DrawRectangle(0, 0, screenWidth, screenHeight, Fade(BLACK, 0.8f));

        if (gameState == GAME_OVER) {
            DrawText("GAME OVER!", screenWidth / 2 - 100, screenHeight / 2 - 60, 30, RED);
            DrawText(TextFormat("Final Score: %d", score), screenWidth / 2 - 90, screenHeight / 2 - 10, 25, WHITE);
            DrawText("Press R to restart", screenWidth / 2 - 100, screenHeight / 2 + 80, 20, GREEN);
        }
        else if (gameState == GAME_WON) {
            DrawText("YOU WIN!", screenWidth / 2 - 80, screenHeight / 2 - 60, 40, GREEN);
            DrawText(TextFormat("Final Score: %d", score), screenWidth / 2 - 90, screenHeight / 2, 25, WHITE);
            DrawText("Press R to restart", screenWidth / 2 - 100, screenHeight / 2 + 80, 20, GREEN);
        }

        DrawText("Press M for Main Menu", screenWidth / 2 - 120, screenHeight / 2 + 120, 20, SKYBLUE);
    }

    void drawMinimalConnections() {
        for (size_t i = 0; i < balls.size(); i++) {
            if (!balls[i].active || !balls[i].isStuck) continue;

            for (size_t j = i + 1; j < balls.size(); j++) {
                if (!balls[j].active || !balls[j].isStuck) continue;

                float dx = balls[j].position.x - balls[i].position.x;
                float dy = balls[j].position.y - balls[i].position.y;
                float distance = sqrtf(dx * dx + dy * dy);

                if (distance < ballRadius * 2.1f) {
                    float alpha = 1.0f - (distance / (ballRadius * 2.1f));
                    DrawLineV(balls[i].position, balls[j].position, Fade(WHITE, alpha * 0.2f));
                }
            }
        }
    }

    void run() {
        while (!WindowShouldClose()) {
            if (IsKeyPressed(KEY_R)) {
                restart();
            }

            if (IsKeyPressed(KEY_M)) {
                if (gameState == GAME_OVER || gameState == GAME_WON) {
                    gameState = MAIN_MENU;
                    restart();
                }
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
        gameState = PLAYING;
        createInitialBalls();
        createNewBall();
    }
};

int main() {
    BallGame game;
    game.run();
    return 0;
}