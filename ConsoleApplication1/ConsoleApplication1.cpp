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
    const float shootSpeed = 20.0f;
    const float gravity = 0.05f;
    const float friction = 0.97f;
    const float minVelocity = 0.2f;

    // Game area boundaries
    const float gameAreaLeft = 10.0f;
    const float gameAreaTop = 50.0f;
    const float gameAreaRight = 790.0f;
    const float gameAreaBottom = 550.0f;
    const float gameAreaWidth = 780.0f;
    const float gameAreaHeight = 500.0f;

    std::vector<Ball> balls;
    Ball* currentBall;
    bool isAiming;
    Vector2 aimDirection;
    int score;
    bool gameOver;

    std::vector<Color> ballColors = {
        RED, BLUE, GREEN, YELLOW, PURPLE, ORANGE, PINK, SKYBLUE, LIME, VIOLET
    };

public:
    BallGame() : isAiming(false), score(0), gameOver(false), currentBall(nullptr) {
        InitWindow(screenWidth, screenHeight, "BubbleBlast");
        SetTargetFPS(60);

        createInitialBalls();
        createNewBall();
    }

    ~BallGame() {
        if (currentBall) delete currentBall;
        CloseWindow();
    }

    void createInitialBalls() {
        // Calculate how many balls fit horizontally in the game area
        int ballsPerRow = (int)((gameAreaWidth - 20) / (ballRadius * 2 + 2));
        int rows = 7;

        // Create a 2D grid to track ball colors for collision detection
        std::vector<std::vector<Color>> colorGrid(rows, std::vector<Color>(ballsPerRow, BLACK));

        // Fill the grid with safe colors
        for (int row = 0; row < rows; row++) {
            for (int col = 0; col < ballsPerRow; col++) {
                colorGrid[row][col] = getColorForPosition(colorGrid, row, col);
            }
        }

        // Create balls using the safe color grid
        for (int row = 0; row < rows; row++) {
            for (int col = 0; col < ballsPerRow; col++) {
                float x = gameAreaLeft + 10 + col * (ballRadius * 2 + 2);
                float y = gameAreaTop + 10 + row * (ballRadius * 2 + 2);

                if (x + ballRadius < gameAreaRight && y + ballRadius < gameAreaBottom) {
                    balls.emplace_back(x, y, ballRadius, colorGrid[row][col]);
                }
            }
        }
    }

    Color getColorForPosition(std::vector<std::vector<Color>>& grid, int row, int col) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> colorDist(0, ballColors.size() - 1);

        // Try multiple times to find a safe color
        for (int attempt = 0; attempt < 50; attempt++) {
            Color candidate = ballColors[colorDist(gen)];

            if (isColorSafe(grid, row, col, candidate)) {
                return candidate;
            }
        }

        // If no safe color found after many attempts, use fallback
        return getFallbackColor(grid, row, col);
    }

    bool isColorSafe(std::vector<std::vector<Color>>& grid, int row, int col, Color color) {
        // Check horizontal: don't create 3 in a row
        if (col >= 2) {
            Color left1 = grid[row][col - 1];
            Color left2 = grid[row][col - 2];
            if (colorsEqual(color, left1) && colorsEqual(color, left2)) {
                return false;
            }
        }

        // Check vertical: don't create 3 in a column
        if (row >= 2) {
            Color above1 = grid[row - 1][col];
            Color above2 = grid[row - 2][col];
            if (colorsEqual(color, above1) && colorsEqual(color, above2)) {
                return false;
            }
        }

        // Check immediate neighbors to prevent potential future triples
        if (col >= 1) {
            Color left = grid[row][col - 1];
            if (colorsEqual(color, left)) {
                // Check if this would set up a potential triple with the next ball
                if (col >= 2 && colorsEqual(left, grid[row][col - 2])) {
                    return false; // Would complete a triple
                }
            }
        }

        if (row >= 1) {
            Color above = grid[row - 1][col];
            if (colorsEqual(color, above)) {
                // Check if this would set up a potential triple with the next ball
                if (row >= 2 && colorsEqual(above, grid[row - 2][col])) {
                    return false; // Would complete a triple
                }
            }
        }

        return true;
    }

    Color getFallbackColor(std::vector<std::vector<Color>>& grid, int row, int col) {
        std::random_device rd;
        std::mt19937 gen(rd());

        // Try to find any color that's different from immediate neighbors
        for (int i = 0; i < ballColors.size(); i++) {
            Color candidate = ballColors[i];
            bool safeFromImmediate = true;

            // Check left neighbor
            if (col >= 1 && colorsEqual(candidate, grid[row][col - 1])) {
                safeFromImmediate = false;
            }

            // Check above neighbor  
            if (row >= 1 && colorsEqual(candidate, grid[row - 1][col])) {
                safeFromImmediate = false;
            }

            if (safeFromImmediate) {
                return candidate;
            }
        }

        // Last resort: completely random
        std::uniform_int_distribution<> colorDist(0, ballColors.size() - 1);
        return ballColors[colorDist(gen)];
    }

    void createNewBall() {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> colorDist(0, ballColors.size() - 1);

        if (currentBall) {
            delete currentBall;
        }
        currentBall = new Ball(screenWidth / 2, gameAreaBottom - 50, ballRadius,
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

        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            shootBall();
        }
    }

    void shootBall() {
        float powerMultiplier = 1.0f;
        float mouseDistance = sqrt(aimDirection.x * aimDirection.x + aimDirection.y * aimDirection.y);
        if (mouseDistance > 100.0f) {
            powerMultiplier = 1.5f;
        }

        currentBall->velocity = {
            aimDirection.x * shootSpeed * powerMultiplier,
            aimDirection.y * shootSpeed * powerMultiplier
        };
        currentBall->isStuck = false;
        isAiming = false;
    }

    void updatePhysics() {
        // Update physics for current ball
        if (currentBall && !currentBall->isStuck) {
            currentBall->velocity.y += gravity;
            currentBall->velocity.x *= friction;
            currentBall->velocity.y *= friction;

            currentBall->position.x += currentBall->velocity.x;
            currentBall->position.y += currentBall->velocity.y;

            // Collision with game area boundaries
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
            else if (currentBall->position.y + currentBall->radius > gameAreaBottom) {
                currentBall->position.y = gameAreaBottom - currentBall->radius;
                currentBall->velocity.y *= -0.9f;

                if (fabs(currentBall->velocity.y) < 2.0f) {
                    currentBall->velocity.y = -3.0f;
                }
            }
        }

        // Update unfixed balls in array
        for (auto& ball : balls) {
            if (!ball.isStuck && ball.active) {
                ball.velocity.y += gravity;
                ball.velocity.x *= friction;
                ball.velocity.y *= friction;

                ball.position.x += ball.velocity.x;
                ball.position.y += ball.velocity.y;

                // Collision with game area boundaries
                if (ball.position.x - ball.radius < gameAreaLeft) {
                    ball.position.x = gameAreaLeft + ball.radius;
                    ball.velocity.x *= -0.7f;
                }
                else if (ball.position.x + ball.radius > gameAreaRight) {
                    ball.position.x = gameAreaRight - ball.radius;
                    ball.velocity.x *= -0.7f;
                }

                if (ball.position.y - ball.radius < gameAreaTop) {
                    ball.position.y = gameAreaTop + ball.radius;
                    ball.velocity.y *= -0.7f;
                }
                else if (ball.position.y + ball.radius > gameAreaBottom) {
                    ball.position.y = gameAreaBottom - ball.radius;
                    ball.velocity.y *= -0.9f;

                    if (fabs(ball.velocity.y) < 2.0f) {
                        ball.velocity.y = -3.0f;
                    }
                }
            }
        }
    }

    void checkCollisions() {
        if (!currentBall || currentBall->isStuck) return;

        bool hasCollision = false;
        Ball* collidedBall = nullptr;

        for (auto& ball : balls) {
            if (!ball.active || !ball.isStuck) continue;

            float dx = currentBall->position.x - ball.position.x;
            float dy = currentBall->position.y - ball.position.y;
            float distance = sqrt(dx * dx + dy * dy);

            if (distance < currentBall->radius + ball.radius) {
                hasCollision = true;
                collidedBall = &ball;

                float overlap = (currentBall->radius + ball.radius) - distance;
                if (distance > 0) {
                    currentBall->position.x += (dx / distance) * overlap * 0.5f;
                    currentBall->position.y += (dy / distance) * overlap * 0.5f;

                    if (currentBall->position.x - currentBall->radius < gameAreaLeft) {
                        currentBall->position.x = gameAreaLeft + currentBall->radius;
                    }
                    else if (currentBall->position.x + currentBall->radius > gameAreaRight) {
                        currentBall->position.x = gameAreaRight - currentBall->radius;
                    }
                    if (currentBall->position.y - currentBall->radius < gameAreaTop) {
                        currentBall->position.y = gameAreaTop + currentBall->radius;
                    }
                    else if (currentBall->position.y + currentBall->radius > gameAreaBottom) {
                        currentBall->position.y = gameAreaBottom - currentBall->radius;
                    }
                }
                break;
            }
        }

        bool shouldStick = hasCollision ||
            (fabs(currentBall->velocity.x) < minVelocity &&
                fabs(currentBall->velocity.y) < minVelocity &&
                currentBall->position.y < gameAreaBottom - 50);

        if (shouldStick) {
            currentBall->isStuck = true;
            currentBall->velocity = { 0, 0 };

            if (hasCollision && collidedBall) {
                float dx = currentBall->position.x - collidedBall->position.x;
                float dy = currentBall->position.y - collidedBall->position.y;
                float distance = sqrt(dx * dx + dy * dy);

                if (distance > 0) {
                    float targetDistance = currentBall->radius + collidedBall->radius;
                    currentBall->position.x = collidedBall->position.x + (dx / distance) * targetDistance;
                    currentBall->position.y = collidedBall->position.y + (dy / distance) * targetDistance;

                    if (currentBall->position.x - currentBall->radius < gameAreaLeft) {
                        currentBall->position.x = gameAreaLeft + currentBall->radius;
                    }
                    else if (currentBall->position.x + currentBall->radius > gameAreaRight) {
                        currentBall->position.x = gameAreaRight - currentBall->radius;
                    }
                    if (currentBall->position.y - currentBall->radius < gameAreaTop) {
                        currentBall->position.y = gameAreaTop + currentBall->radius;
                    }
                    else if (currentBall->position.y + currentBall->radius > gameAreaBottom) {
                        currentBall->position.y = gameAreaBottom - currentBall->radius;
                    }
                }
            }

            balls.push_back(*currentBall);
            checkBallGroups();
            createNewBall();
        }

        if (currentBall && currentBall->position.y > gameAreaBottom + 50) {
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
                score += group.size() * 15;

                if (group.size() >= 5) score += group.size() * 10;
                if (group.size() >= 7) score += group.size() * 20;
                if (group.size() >= 10) score += group.size() * 30;
            }
        }

        for (int index : toRemove) {
            balls[index].active = false;
        }

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
        if (balls.size() > 250) {
            gameOver = true;
        }

        float highestY = gameAreaTop;
        for (const auto& ball : balls) {
            if (ball.isStuck && ball.active && ball.position.y > highestY) {
                highestY = ball.position.y;
            }
        }

        if (highestY > gameAreaBottom - 30) {
            gameOver = true;
        }
    }

    void draw() {
        BeginDrawing();
        ClearBackground(BLACK);

        DrawRectangle(0, screenHeight - 50, screenWidth, 50, DARKGRAY);
        DrawRectangle(0, 0, screenWidth, 50, DARKGRAY);

        DrawRectangle(gameAreaLeft, gameAreaTop, gameAreaWidth, gameAreaHeight, Fade(DARKBLUE, 0.1f));
        DrawRectangleLines(gameAreaLeft, gameAreaTop, gameAreaWidth, gameAreaHeight, BLUE);

        for (const auto& ball : balls) {
            if (ball.active) {
                DrawCircleV(ball.position, ball.radius, ball.color);
                DrawCircleLines(ball.position.x, ball.position.y, ball.radius,
                    ball.isStuck ? Fade(WHITE, 0.5f) : Fade(GRAY, 0.5f));
            }
        }

        if (currentBall) {
            DrawCircleV(currentBall->position, currentBall->radius, currentBall->color);
            DrawCircleLines(currentBall->position.x, currentBall->position.y,
                currentBall->radius, YELLOW);

            if (isAiming) {
                Vector2 endPoint = {
                    currentBall->position.x + aimDirection.x * 150,
                    currentBall->position.y + aimDirection.y * 150
                };
                DrawLineV(currentBall->position, endPoint, YELLOW);
                DrawCircleV(endPoint, 5, RED);

                float power = sqrt(aimDirection.x * aimDirection.x + aimDirection.y * aimDirection.y) * 80;
                DrawRectangle(screenWidth - 150, screenHeight - 45, (int)power, 10,
                    power > 60 ? RED : (power > 40 ? ORANGE : GREEN));
            }
        }

        DrawText(TextFormat("Score: %d", score), 20, 15, 20, WHITE);
        DrawText(TextFormat("Balls: %zu", balls.size()), screenWidth - 150, 15, 20, WHITE);
        DrawText("BubbleBlast", screenWidth / 2 - 60, 15, 20, BLUE);

        DrawText("LMB - shoot, R - restart", 20, screenHeight - 30, 15, LIGHTGRAY);
        DrawText("Power:", screenWidth - 180, screenHeight - 45, 15, GREEN);

        if (isAiming) {
            float power = sqrt(aimDirection.x * aimDirection.x + aimDirection.y * aimDirection.y) * 100;
            DrawText(TextFormat("Power: %.0f%%", power), screenWidth / 2 - 50, screenHeight - 100, 18,
                power > 80 ? RED : (power > 50 ? YELLOW : GREEN));
        }

        if (gameOver) {
            DrawRectangle(0, 0, screenWidth, screenHeight, Fade(BLACK, 0.8f));
            DrawText("GAME OVER!", screenWidth / 2 - 100, screenHeight / 2 - 60, 40, RED);
            DrawText(TextFormat("Final Score: %d", score), screenWidth / 2 - 90, screenHeight / 2 - 10, 30, WHITE);
            DrawText(TextFormat("Groups Destroyed: %d", score / 15), screenWidth / 2 - 110, screenHeight / 2 + 30, 25, YELLOW);
            DrawText("Press R to restart", screenWidth / 2 - 100, screenHeight / 2 + 80, 25, GREEN);
        }

        EndDrawing();
    }

    void run() {
        while (!WindowShouldClose()) {
            if (IsKeyPressed(KEY_R)) {
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