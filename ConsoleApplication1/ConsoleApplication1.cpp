#include "raylib.h"
#include <vector>
#include <cmath>
#include <algorithm>
#include <random>
#include <unordered_set>
#include <memory>
#include <string>

enum GameState {
    MAIN_MENU,
    LEVEL_SELECT,
    PLAYING,
    GAME_OVER,
    GAME_WON
};

enum BallType {
    NORMAL,
    UNIVERSAL,
    BOMB,
    RAINBOW
};

struct Level {
    int levelNumber;
    int targetScore;
    int ballCount;
    int specialBallChance;
    bool allowBomb;
    bool allowRainbow;
    bool allowUniversal;
    std::string name;
    Color backgroundColor;
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
    bool hasSupport;
    BallType type;
    bool isSpecial;
    int bombRadius;
    Color originalColor;

    Ball(float x, float y, float r, Color c, BallType t = NORMAL)
        : position{ x, y }, velocity{ 0, 0 }, acceleration{ 0, 0 },
        radius(r), color(c), active(true), isStuck(true),
        stiffness(0.08f), damping(0.92f), originalPosition{ x, y },
        hasSupport(true), type(t), isSpecial(t != NORMAL),
        bombRadius(static_cast<int>(r * 3)), originalColor(c) {

        if (type == UNIVERSAL) {
            color = WHITE;
            originalColor = WHITE;
        }
        else if (type == BOMB) {
            color = BLACK;
            originalColor = BLACK;
        }
        else if (type == RAINBOW) {
            color = RED;
            originalColor = RED;
        }
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
    const float antiGravity = -0.2f;
    const float clusterMagnetStrength = 2.0f;
    const float maxClusterMagnetDistance = 300.0f;

    std::vector<Ball> balls;
    Ball* currentBall;
    bool isAiming;
    Vector2 aimDirection;
    int score;
    GameState gameState;
    int currentLevel;
    std::vector<Level> levels;
    bool isLevelMode;

    Vector2 newBallPosition;

    std::vector<Color> ballColors = {
        RED, BLUE, GREEN, YELLOW, PURPLE, ORANGE, PINK, SKYBLUE, LIME, VIOLET
    };

    int UNIVERSAL_CHANCE = 5;
    int BOMB_CHANCE = 3;
    int RAINBOW_CHANCE = 2;

    struct Particle {
        Vector2 position;
        Vector2 velocity;
        Color color;
        float size;
        float life;
    };

    std::vector<Particle> particles;

    Texture2D menuBackgroundTexture;
    Texture2D gameBackgroundTexture;
    Texture2D startButtonTexture;
    Texture2D exitButtonTexture;
    Texture2D levelsButtonTexture;
    Texture2D logoTexture;
    Texture2D universalIconTexture;
    Texture2D bombIconTexture;
    Texture2D rainbowIconTexture;
    Texture2D backButtonTexture;

    bool texturesLoaded = false;

    Rectangle startButtonRect;
    Rectangle levelsButtonRect;
    Rectangle exitButtonRect;

public:
    BallGame() : isAiming(false), score(0), gameState(MAIN_MENU),
        currentBall(nullptr), currentLevel(1), isLevelMode(false) {
        InitWindow(screenWidth, screenHeight, "BubbleBlast");
        SetTargetFPS(60);

        InitAudioDevice();
        loadTextures();

        initializeLevels();

        newBallPosition = { static_cast<float>(screenWidth) / 2.0f, gameAreaBottom - 30.0f };

        startButtonRect = { screenWidth / 2.0f - 100.0f, screenHeight / 2.0f, 200.0f, 70.0f };
        levelsButtonRect = { screenWidth / 2.0f - 100.0f, screenHeight / 2.0f + 80.0f, 200.0f, 70.0f };
        exitButtonRect = { screenWidth / 2.0f - 100.0f, screenHeight / 2.0f + 160.0f, 200.0f, 70.0f };

        createInitialBalls(false);
        createNewBall();
    }

    ~BallGame() {
        if (currentBall) delete currentBall;

        unloadTextures();
        CloseAudioDevice();
        CloseWindow();
    }

    void initializeLevels() {
        levels.clear();

        levels.push_back({
            1,
            500,
            50,
            0,
            false,
            false,
            false,
            "Tutorial",
            DARKBLUE
            });

        levels.push_back({
            2,
            1000,
            70,
            5,
            true,
            false,
            false,
            "Easy Mode",
            DARKGREEN
            });

        levels.push_back({
            3,
            2000,
            90,
            10,
            true,
            true,
            false,
            "Medium Challenge",
            PURPLE
            });

        levels.push_back({
            4,
            3500,
            110,
            15,
            true,
            true,
            true,
            "Hard Level",
            DARKPURPLE
            });

        levels.push_back({
            5,
            5000,
            130,
            20,
            true,
            true,
            true,
            "Expert Mode",
            MAROON
            });
    }

    void loadTextures() {
        if (FileExists("assets/logo.png")) {
            logoTexture = LoadTexture("assets/logo.png");
        }
        else {
            logoTexture = { 0 };
        }

        if (FileExists("assets/menu_background.png")) {
            menuBackgroundTexture = LoadTexture("assets/menu_background.png");
        }
        else {
            menuBackgroundTexture = { 0 };
        }

        if (FileExists("assets/game_background.png")) {
            gameBackgroundTexture = LoadTexture("assets/game_background.png");
        }
        else {
            gameBackgroundTexture = { 0 };
        }

        if (FileExists("assets/start_button.png")) {
            startButtonTexture = LoadTexture("assets/start_button.png");
        }
        else {
            startButtonTexture = { 0 };
        }

        if (FileExists("assets/levels_button.png")) {
            levelsButtonTexture = LoadTexture("assets/levels_button.png");
        }
        else {
            levelsButtonTexture = { 0 };
        }

        if (FileExists("assets/exit_button.png")) {
            exitButtonTexture = LoadTexture("assets/exit_button.png");
        }
        else {
            exitButtonTexture = { 0 };
        }

        if (FileExists("assets/universal_icon.png")) {
            universalIconTexture = LoadTexture("assets/universal_icon.png");
        }
        else {
            universalIconTexture = { 0 };
        }

        if (FileExists("assets/bomb_icon.png")) {
            bombIconTexture = LoadTexture("assets/bomb_icon.png");
        }
        else {
            bombIconTexture = { 0 };
        }

        if (FileExists("assets/rainbow_icon.png")) {
            rainbowIconTexture = LoadTexture("assets/rainbow_icon.png");
        }
        else {
            rainbowIconTexture = { 0 };
        }

        if (FileExists("assets/back_button.png")) {
            backButtonTexture = LoadTexture("assets/back_button.png");
        }
        else {
            backButtonTexture = { 0 };
        }

        texturesLoaded = true;
    }

    void unloadTextures() {
        if (logoTexture.id != 0) UnloadTexture(logoTexture);
        if (menuBackgroundTexture.id != 0) UnloadTexture(menuBackgroundTexture);
        if (gameBackgroundTexture.id != 0) UnloadTexture(gameBackgroundTexture);
        if (startButtonTexture.id != 0) UnloadTexture(startButtonTexture);
        if (levelsButtonTexture.id != 0) UnloadTexture(levelsButtonTexture);
        if (exitButtonTexture.id != 0) UnloadTexture(exitButtonTexture);
        if (universalIconTexture.id != 0) UnloadTexture(universalIconTexture);
        if (bombIconTexture.id != 0) UnloadTexture(bombIconTexture);
        if (rainbowIconTexture.id != 0) UnloadTexture(rainbowIconTexture);
        if (backButtonTexture.id != 0) UnloadTexture(backButtonTexture);
    }

    void createInitialBalls(bool isLevel) {
        balls.clear();

        if (isLevel) {
            if (currentLevel < 1 || currentLevel > static_cast<int>(levels.size())) {
                currentLevel = 1;
            }

            Level& level = levels[static_cast<size_t>(currentLevel) - 1];

            UNIVERSAL_CHANCE = level.allowUniversal ? 5 : 0;
            BOMB_CHANCE = level.allowBomb ? 3 : 0;
            RAINBOW_CHANCE = level.allowRainbow ? 2 : 0;

            int ballsPerRow = static_cast<int>(gameAreaWidth / (ballRadius * 2.0f));
            int rows = static_cast<int>(level.ballCount / ballsPerRow) + 1;

            std::vector<std::vector<Color>> colorGrid(static_cast<size_t>(rows),
                std::vector<Color>(static_cast<size_t>(ballsPerRow), BLACK));

            for (int row = 0; row < rows; row++) {
                for (int col = 0; col < ballsPerRow; col++) {
                    colorGrid[static_cast<size_t>(row)][static_cast<size_t>(col)] = getColorForPosition(colorGrid, row, col);
                }
            }

            float totalWidth = static_cast<float>(ballsPerRow) * ballRadius * 2.0f;
            float startX = gameAreaLeft + (gameAreaWidth - totalWidth) / 2.0f + ballRadius;

            int ballsCreated = 0;
            for (int row = 0; row < rows && ballsCreated < level.ballCount; row++) {
                for (int col = 0; col < ballsPerRow && ballsCreated < level.ballCount; col++) {
                    float x = startX + static_cast<float>(col) * (ballRadius * 2.0f);
                    float y = gameAreaTop + 10.0f + static_cast<float>(row) * (ballRadius * 2.0f);

                    if (x + ballRadius < gameAreaRight && y + ballRadius < gameAreaBottom) {
                        balls.emplace_back(x, y, ballRadius, colorGrid[static_cast<size_t>(row)][static_cast<size_t>(col)]);
                        balls.back().hasSupport = (row == 0);
                        ballsCreated++;
                    }
                }
            }
        }
        else {
            UNIVERSAL_CHANCE = 5;
            BOMB_CHANCE = 3;
            RAINBOW_CHANCE = 2;

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
                        balls.back().hasSupport = (row == 0);
                    }
                }
            }
        }
    }

    BallType getRandomBallType() {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> chanceDist(0, 99);

        int chance = chanceDist(gen);

        if (chance < RAINBOW_CHANCE) {
            return RAINBOW;
        }
        else if (chance < RAINBOW_CHANCE + BOMB_CHANCE) {
            return BOMB;
        }
        else if (chance < RAINBOW_CHANCE + BOMB_CHANCE + UNIVERSAL_CHANCE) {
            return UNIVERSAL;
        }

        return NORMAL;
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
            currentBall = nullptr;
        }

        BallType ballType = getRandomBallType();
        Color ballColor = ballColors[static_cast<size_t>(colorDist(gen))];

        currentBall = new Ball(newBallPosition.x, newBallPosition.y, ballRadius,
            ballColor, ballType);
        currentBall->isStuck = false;
        currentBall->originalPosition = newBallPosition;
        currentBall->hasSupport = true;
        isAiming = true;
    }

    void update() {
        updateParticles();

        if (gameState == MAIN_MENU) {
            updateMainMenu();
        }
        else if (gameState == LEVEL_SELECT) {
            updateLevelSelect();
        }
        else if (gameState == PLAYING) {
            updateGame();
        }
    }

    void updateParticles() {
        for (auto it = particles.begin(); it != particles.end(); ) {
            it->position.x += it->velocity.x;
            it->position.y += it->velocity.y;
            it->life -= 0.02f;
            it->size *= 0.98f;

            if (it->life <= 0.0f) {
                it = particles.erase(it);
            }
            else {
                ++it;
            }
        }
    }

    void createExplosion(Vector2 position, Color color, int count = 30) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<float> dist(-3.0f, 3.0f);
        std::uniform_real_distribution<float> lifeDist(0.5f, 1.5f);

        for (int i = 0; i < count; i++) {
            Particle p;
            p.position = position;
            p.velocity = { dist(gen), dist(gen) };
            p.color = color;
            p.size = 3.0f + static_cast<float>(rand() % 5);
            p.life = lifeDist(gen);
            particles.push_back(p);
        }
    }

    void updateMainMenu() {
        Vector2 mousePoint = GetMousePosition();

        if (CheckCollisionPointRec(mousePoint, startButtonRect)) {
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                isLevelMode = false;
                gameState = PLAYING;
                restart();
            }
        }

        if (CheckCollisionPointRec(mousePoint, levelsButtonRect)) {
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                gameState = LEVEL_SELECT;
            }
        }

        if (CheckCollisionPointRec(mousePoint, exitButtonRect)) {
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                CloseWindow();
            }
        }
    }

    void updateLevelSelect() {
        Vector2 mousePoint = GetMousePosition();

        float buttonSize = 60.0f;
        float buttonMargin = 20.0f;
        Rectangle backButtonRect = {
            screenWidth - buttonSize - buttonMargin,
            screenHeight - buttonSize - buttonMargin,
            buttonSize,
            buttonSize
        };

        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            if (CheckCollisionPointRec(mousePoint, backButtonRect)) {
                gameState = MAIN_MENU;
                return;
            }

            int levelButtonHeight = 70;
            int startY = 100;

            for (size_t i = 0; i < levels.size(); i++) {
                Rectangle levelRect = { 50.0f, startY + static_cast<float>(i) * (levelButtonHeight + 10.0f),
                                      screenWidth - 100.0f, static_cast<float>(levelButtonHeight) };

                if (CheckCollisionPointRec(mousePoint, levelRect)) {
                    isLevelMode = true;
                    currentLevel = static_cast<int>(i) + 1;
                    gameState = PLAYING;
                    restart();
                    break;
                }
            }
        }

        if (IsKeyPressed(KEY_ESCAPE)) {
            gameState = MAIN_MENU;
        }
    }

    void updateGame() {
        updateRainbowBalls();

        if (isAiming) {
            handleAiming();
            updateBallPhysics();
        }
        else {
            updatePhysics();
            checkCollisions();
            updateBallPhysics();
            checkSupport();
            applyClusterMagnetForces();
            applyAntiGravity();
            checkGameOver();
            if (isLevelMode) {
                checkLevelComplete();
            }
        }
    }

    void updateRainbowBalls() {
        static float rainbowTimer = 0.0f;
        rainbowTimer += GetFrameTime();

        if (rainbowTimer > 0.1f) {
            rainbowTimer = 0.0f;

            for (auto& ball : balls) {
                if (ball.type == RAINBOW && ball.active) {
                    if (ball.color.r == 255 && ball.color.g == 0 && ball.color.b == 0) ball.color = ORANGE;
                    else if (ball.color.r == 255 && ball.color.g < 255 && ball.color.b == 0) ball.color = YELLOW;
                    else if (ball.color.r == 255 && ball.color.g == 255 && ball.color.b == 0) ball.color = GREEN;
                    else if (ball.color.r == 0 && ball.color.g == 255 && ball.color.b == 0) ball.color = SKYBLUE;
                    else if (ball.color.r == 0 && ball.color.g == 255 && ball.color.b == 255) ball.color = BLUE;
                    else if (ball.color.r == 0 && ball.color.g == 0 && ball.color.b == 255) ball.color = PURPLE;
                    else if (ball.color.r == 255 && ball.color.g == 0 && ball.color.b == 255) ball.color = RED;
                    ball.originalColor = ball.color;
                }
            }
        }
    }

    void handleAiming() {
        if (!currentBall) return;

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

        if (targetPosition.y - ballRadius < gameAreaTop) {
            targetPosition.y = gameAreaTop + ballRadius;
        }
        else if (targetPosition.y + ballRadius > gameAreaBottom) {
            targetPosition.y = gameAreaBottom - ballRadius;
        }

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
        if (!currentBall) return;

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
        currentBall->hasSupport = false;
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

    void applyClusterMagnetForces() {
        Vector2 clusterCenter = { 0.0f, 0.0f };
        int clusterCount = 0;

        for (auto& ball : balls) {
            if (!ball.active || !ball.isStuck || !ball.hasSupport) continue;

            clusterCenter.x += ball.position.x;
            clusterCenter.y += ball.position.y;
            clusterCount++;
        }

        if (clusterCount == 0) {
            clusterCenter = { screenWidth / 2.0f, gameAreaBottom - 100.0f };
            clusterCount = 1;
        }
        else {
            clusterCenter.x /= clusterCount;
            clusterCenter.y /= clusterCount;
        }

        for (auto& ball : balls) {
            if (!ball.active || !ball.isStuck || ball.hasSupport) continue;

            float dx = clusterCenter.x - ball.position.x;
            float dy = clusterCenter.y - ball.position.y;
            float distance = sqrtf(dx * dx + dy * dy);

            if (distance > ballRadius * 2.0f) {
                float force = clusterMagnetStrength * (0.5f + distance / 100.0f);

                if (distance > 100.0f) force *= 2.0f;

                float forceX = (dx / distance) * force;
                float forceY = (dy / distance) * force;

                ball.velocity.x += forceX;
                ball.velocity.y += forceY;

                float speed = sqrtf(ball.velocity.x * ball.velocity.x + ball.velocity.y * ball.velocity.y);
                if (speed > maxBallSpeed * 3.0f) {
                    ball.velocity.x = (ball.velocity.x / speed) * maxBallSpeed * 3.0f;
                    ball.velocity.y = (ball.velocity.y / speed) * maxBallSpeed * 3.0f;
                }
            }
        }

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

    void checkSupport() {
        for (auto& ball : balls) {
            if (!ball.active || !ball.isStuck) continue;
            ball.hasSupport = false;
        }

        for (auto& ball : balls) {
            if (!ball.active || !ball.isStuck) continue;
            if (ball.position.y - ball.radius <= gameAreaTop + 1.0f) {
                ball.hasSupport = true;
            }
        }

        bool changed;
        int maxIterations = 1000;
        int iterations = 0;

        do {
            changed = false;
            iterations++;

            for (auto& ball : balls) {
                if (!ball.active || !ball.isStuck || ball.hasSupport) continue;

                for (const auto& other : balls) {
                    if (!other.active || !other.isStuck || !other.hasSupport) continue;

                    float dx = other.position.x - ball.position.x;
                    float dy = other.position.y - ball.position.y;
                    float distance = sqrtf(dx * dx + dy * dy);

                    if (distance < ballRadius * 2.2f) {
                        ball.hasSupport = true;
                        changed = true;
                        break;
                    }
                }
            }

            if (iterations >= maxIterations) {
                break;
            }
        } while (changed);
    }

    void applyAntiGravity() {
        for (auto& ball : balls) {
            if (!ball.active || !ball.isStuck || ball.hasSupport) continue;

            ball.velocity.y += antiGravity * 0.3f;

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
            else if (ball.position.x + ballRadius > gameAreaRight - margin) {
                ball.position.x = gameAreaRight - ball.radius - margin;
                ball.velocity.x = 0.0f;
            }

            if (ball.position.y - ball.radius < gameAreaTop) {
                ball.position.y = gameAreaTop + ball.radius;
                ball.velocity.y = 0.0f;
                ball.hasSupport = true;
            }

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
            currentBall->hasSupport = closestBall->hasSupport;

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

            if (currentBall->type == BOMB) {
                activateBomb(*currentBall);
                delete currentBall;
                currentBall = nullptr;
                createNewBall();
                return;
            }
            else if (currentBall->type == RAINBOW) {
                balls.push_back(*currentBall);
                currentBall = nullptr;
            }
            else {
                balls.push_back(*currentBall);
                currentBall = nullptr;
            }

            checkBallGroups();

            createNewBall();
        }

        if (currentBall && !currentBall->isStuck) {
            if (currentBall->position.y > gameAreaBottom + 50.0f ||
                currentBall->position.y < gameAreaTop - 50.0f ||
                currentBall->position.x < gameAreaLeft - 50.0f ||
                currentBall->position.x > gameAreaRight + 50.0f) {

                if (currentBall) {
                    delete currentBall;
                    currentBall = nullptr;
                }
                createNewBall();
            }
        }
    }

    void handleSpecialBallCollision(Ball& specialBall) {
        switch (specialBall.type) {
        case UNIVERSAL:
            break;

        case BOMB:
            activateBomb(specialBall);
            break;

        case RAINBOW:
            break;

        case NORMAL:
        default:
            break;
        }
    }

    void activateBomb(Ball& bomb) {
        createExplosion(bomb.position, YELLOW, 50);

        std::vector<size_t> toRemove;
        for (size_t i = 0; i < balls.size(); i++) {
            if (!balls[i].active) continue;

            float dx = balls[i].position.x - bomb.position.x;
            float dy = balls[i].position.y - bomb.position.y;
            float distance = sqrtf(dx * dx + dy * dy);

            if (distance < bomb.bombRadius) {
                toRemove.push_back(i);
            }
        }

        for (size_t index : toRemove) {
            balls[index].active = false;
            createExplosion(balls[index].position, RED, 10);
        }

        balls.erase(std::remove_if(balls.begin(), balls.end(),
            [](const Ball& ball) { return !ball.active; }),
            balls.end());

        score += static_cast<int>(toRemove.size()) * 20;

        applyGentleRemovalImpulse();
    }

    void activateRainbow(Ball& rainbowBall) {
        createExplosion(rainbowBall.position, rainbowBall.color, 40);

        std::vector<size_t> toRemove;
        Color targetColor = rainbowBall.originalColor;

        for (size_t i = 0; i < balls.size(); i++) {
            if (!balls[i].active) continue;

            if (colorsEqual(balls[i].color, targetColor)) {
                toRemove.push_back(i);
            }
        }

        for (size_t index : toRemove) {
            balls[index].active = false;
            createExplosion(balls[index].position, targetColor, 5);
        }

        for (size_t i = 0; i < balls.size(); i++) {
            if (&balls[i] == &rainbowBall) {
                balls[i].active = false;
                break;
            }
        }

        balls.erase(std::remove_if(balls.begin(), balls.end(),
            [](const Ball& ball) { return !ball.active; }),
            balls.end());

        score += static_cast<int>(toRemove.size()) * 25;

        applyGentleRemovalImpulse();
    }

    void checkBallGroups() {
        if (balls.empty()) return;

        std::vector<int> toRemove;
        std::vector<bool> visited(balls.size(), false);

        for (size_t i = 0; i < balls.size(); i++) {
            if (!balls[i].active || visited[i] || !balls[i].isStuck) continue;

            std::vector<int> group;
            findConnectedBalls(static_cast<int>(i), group, balls[i].color, visited, balls[i].type);

            if (group.size() >= 4 || balls[i].type == UNIVERSAL) {
                if (balls[i].type == RAINBOW && group.size() >= 4) {
                    activateRainbow(balls[static_cast<size_t>(group[0])]);
                    return;
                }

                toRemove.insert(toRemove.end(), group.begin(), group.end());
                score += static_cast<int>(group.size()) * 15;

                if (group.size() >= 5) score += static_cast<int>(group.size()) * 10;
                if (group.size() >= 7) score += static_cast<int>(group.size()) * 20;
                if (group.size() >= 10) score += static_cast<int>(group.size()) * 30;

                if (group.size() == 4) {
                    score += 25;
                }

                if (balls[i].type == UNIVERSAL) {
                    score += 50;
                }
            }
        }

        for (int index : toRemove) {
            balls[static_cast<size_t>(index)].active = false;
            createExplosion(balls[static_cast<size_t>(index)].position,
                balls[static_cast<size_t>(index)].color, 5);
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

    void findConnectedBalls(int startIndex, std::vector<int>& group, Color targetColor,
        std::vector<bool>& visited, BallType ballType) {
        if (visited[static_cast<size_t>(startIndex)]) return;

        visited[static_cast<size_t>(startIndex)] = true;
        group.push_back(startIndex);

        for (size_t i = 0; i < balls.size(); i++) {
            if (visited[i] || !balls[i].active || !balls[i].isStuck) continue;

            bool colorMatches = false;
            if (ballType == UNIVERSAL) {
                colorMatches = true;
            }
            else if (balls[i].type == UNIVERSAL) {
                colorMatches = true;
            }
            else if (ballType == RAINBOW) {
                colorMatches = true;
            }
            else if (balls[i].type == RAINBOW) {
                colorMatches = true;
            }
            else {
                colorMatches = colorsEqual(balls[i].color, targetColor);
            }

            if (colorMatches) {
                float dx = balls[i].position.x - balls[static_cast<size_t>(startIndex)].position.x;
                float dy = balls[i].position.y - balls[static_cast<size_t>(startIndex)].position.y;
                float distance = sqrtf(dx * dx + dy * dy);

                if (distance < ballRadius * 2.2f) {
                    findConnectedBalls(static_cast<int>(i), group, targetColor, visited, ballType);
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

    void checkLevelComplete() {
        if (currentLevel < 1 || currentLevel > static_cast<int>(levels.size())) {
            return;
        }

        Level& level = levels[static_cast<size_t>(currentLevel) - 1];

        if (score >= level.targetScore) {
            if (currentLevel < static_cast<int>(levels.size())) {
                currentLevel++;
                gameState = PLAYING;
                restart();
            }
            else {
                gameState = GAME_WON;
            }
        }
    }

    void draw() {
        BeginDrawing();

        if (gameState == MAIN_MENU) {
            drawMainMenu();
        }
        else if (gameState == LEVEL_SELECT) {
            drawLevelSelect();
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

    void drawParticles() {
        for (const auto& particle : particles) {
            DrawCircleV(particle.position, particle.size, Fade(particle.color, particle.life));
        }
    }

    void drawMainMenu() {
        if (menuBackgroundTexture.id != 0) {
            DrawTexturePro(menuBackgroundTexture,
                { 0, 0, (float)menuBackgroundTexture.width, (float)menuBackgroundTexture.height },
                { 0, 0, (float)screenWidth, (float)screenHeight },
                { 0, 0 }, 0.0f, WHITE);
        }
        else {
            DrawRectangleGradientV(0, 0, screenWidth, screenHeight, DARKBLUE, BLACK);
        }

        if (logoTexture.id != 0) {
            float logoWidth = 300.0f;
            float logoHeight = 150.0f;

            Rectangle logoRect = {
                screenWidth / 2.0f - logoWidth / 2.0f,
                120.0f,
                logoWidth,
                logoHeight
            };

            DrawTexturePro(logoTexture,
                { 0, 0, (float)logoTexture.width, (float)logoTexture.height },
                logoRect,
                { 0, 0 }, 0.0f, WHITE);
        }
        else {
            DrawText("BubbleBlast", screenWidth / 2 - MeasureText("BubbleBlast", 50) / 2, 150, 50, WHITE);
        }

        Color startTint = WHITE;
        if (CheckCollisionPointRec(GetMousePosition(), startButtonRect)) {
            startTint = YELLOW;
        }

        if (startButtonTexture.id != 0) {
            DrawTexturePro(startButtonTexture,
                { 0, 0, (float)startButtonTexture.width, (float)startButtonTexture.height },
                startButtonRect,
                { 0, 0 }, 0.0f, startTint);
        }
        else {
            DrawRectangleRec(startButtonRect, LIGHTGRAY);
            DrawRectangleLinesEx(startButtonRect, 2, DARKGRAY);
            DrawText("Start Game",
                startButtonRect.x + startButtonRect.width / 2 - MeasureText("Start Game", 20) / 2,
                startButtonRect.y + startButtonRect.height / 2 - 10,
                20, DARKBLUE);
        }

        Color levelsTint = WHITE;
        if (CheckCollisionPointRec(GetMousePosition(), levelsButtonRect)) {
            levelsTint = YELLOW;
        }

        if (levelsButtonTexture.id != 0) {
            DrawTexturePro(levelsButtonTexture,
                { 0, 0, (float)levelsButtonTexture.width, (float)levelsButtonTexture.height },
                levelsButtonRect,
                { 0, 0 }, 0.0f, levelsTint);
        }
        else {
            DrawRectangleRec(levelsButtonRect, LIGHTGRAY);
            DrawRectangleLinesEx(levelsButtonRect, 2, DARKGRAY);
            DrawText("Levels",
                levelsButtonRect.x + levelsButtonRect.width / 2 - MeasureText("Levels", 20) / 2,
                levelsButtonRect.y + levelsButtonRect.height / 2 - 10,
                20, DARKBLUE);
        }

        Color exitTint = WHITE;
        if (CheckCollisionPointRec(GetMousePosition(), exitButtonRect)) {
            exitTint = YELLOW;
        }

        if (exitButtonTexture.id != 0) {
            DrawTexturePro(exitButtonTexture,
                { 0, 0, (float)exitButtonTexture.width, (float)exitButtonTexture.height },
                exitButtonRect,
                { 0, 0 }, 0.0f, exitTint);
        }
        else {
            DrawRectangleRec(exitButtonRect, LIGHTGRAY);
            DrawRectangleLinesEx(exitButtonRect, 2, DARKGRAY);
            DrawText("Exit",
                exitButtonRect.x + exitButtonRect.width / 2 - MeasureText("Exit", 20) / 2,
                exitButtonRect.y + exitButtonRect.height / 2 - 10,
                20, DARKBLUE);
        }
    }

    void drawLevelSelect() {
        DrawRectangleGradientV(0, 0, screenWidth, screenHeight, DARKPURPLE, GRAY);

        DrawText("SELECT LEVEL", screenWidth / 2 - MeasureText("SELECT LEVEL", 40) / 2, 30, 40, WHITE);

        float buttonSize = 60.0f;
        float buttonMargin = 20.0f;
        Rectangle backButtonRect = {
            screenWidth - buttonSize - buttonMargin,
            screenHeight - buttonSize - buttonMargin,
            buttonSize,
            buttonSize
        };

        Color backButtonColor = GRAY;
        if (CheckCollisionPointRec(GetMousePosition(), backButtonRect)) {
            backButtonColor = YELLOW;
            DrawRectangleLinesEx(backButtonRect, 3, WHITE);
        }

        if (backButtonTexture.id != 0) {
            DrawTexturePro(backButtonTexture,
                { 0, 0, (float)backButtonTexture.width, (float)backButtonTexture.height },
                backButtonRect,
                { 0, 0 }, 0.0f, WHITE);
        }
        else {
            DrawRectangleRec(backButtonRect, backButtonColor);
            DrawRectangleLinesEx(backButtonRect, 2, WHITE);

            Vector2 arrowPoint1 = { backButtonRect.x + backButtonRect.width * 0.7f, backButtonRect.y + backButtonRect.height * 0.3f };
            Vector2 arrowPoint2 = { backButtonRect.x + backButtonRect.width * 0.7f, backButtonRect.y + backButtonRect.height * 0.7f };
            Vector2 arrowPoint3 = { backButtonRect.x + backButtonRect.width * 0.3f, backButtonRect.y + backButtonRect.height * 0.5f };

            DrawTriangle(arrowPoint1, arrowPoint2, arrowPoint3, BLACK);
        }

        int levelButtonHeight = 70;
        int startY = 100;

        for (size_t i = 0; i < levels.size(); i++) {
            const Level& level = levels[i];

            Color buttonColor;
            if (i % 5 == 0) buttonColor = BLUE;
            else if (i % 5 == 1) buttonColor = DARKGREEN;
            else if (i % 5 == 2) buttonColor = ORANGE;
            else if (i % 5 == 3) buttonColor = MAGENTA;
            else buttonColor = RED;

            if (currentLevel == static_cast<int>(i) + 1) {
                buttonColor = Fade(buttonColor, 0.7f);
            }

            Rectangle levelRect = { 50.0f, startY + static_cast<float>(i) * (levelButtonHeight + 10.0f),
                                  screenWidth - 100.0f, static_cast<float>(levelButtonHeight) };

            if (CheckCollisionPointRec(GetMousePosition(), levelRect)) {
                buttonColor = Fade(buttonColor, 0.8f);
                DrawRectangleLinesEx(levelRect, 3, YELLOW);
            }

            DrawRectangleRec(levelRect, buttonColor);
            DrawRectangleLinesEx(levelRect, 2, WHITE);

            std::string levelText = "Level " + std::to_string(level.levelNumber) + ": " + level.name;
            DrawText(levelText.c_str(),
                static_cast<int>(levelRect.x + 20),
                static_cast<int>(levelRect.y + 10),
                22, WHITE);

            std::string scoreText = "Target: " + std::to_string(level.targetScore) + " points";
            DrawText(scoreText.c_str(),
                static_cast<int>(levelRect.x + 20),
                static_cast<int>(levelRect.y + 35),
                16, LIGHTGRAY);

            std::string difficulty = "";
            if (i == 0) difficulty = "★☆☆☆☆";
            else if (i == 1) difficulty = "★★☆☆☆";
            else if (i == 2) difficulty = "★★★☆☆";
            else if (i == 3) difficulty = "★★★★☆";
            else difficulty = "★★★★★";

            DrawText(difficulty.c_str(),
                static_cast<int>(levelRect.x + levelRect.width - 70),
                static_cast<int>(levelRect.y + 25),
                20, YELLOW);
        }
    }

    void drawGame() {
        if (isLevelMode && currentLevel >= 1 && currentLevel <= static_cast<int>(levels.size())) {
            Level& level = levels[static_cast<size_t>(currentLevel) - 1];
            DrawRectangle(0, 0, screenWidth, screenHeight, level.backgroundColor);
        }
        else if (gameBackgroundTexture.id != 0) {
            DrawTexturePro(gameBackgroundTexture,
                { 0, 0, (float)gameBackgroundTexture.width, (float)gameBackgroundTexture.height },
                { 0, 0, (float)screenWidth, (float)screenHeight },
                { 0, 0 }, 0.0f, WHITE);
        }
        else {
            ClearBackground(BLACK);
        }

        drawParticles();

        DrawRectangle(0, screenHeight - 50, screenWidth, 50, Fade(DARKGRAY, 0.7f));
        DrawRectangle(0, 0, screenWidth, 60, Fade(DARKGRAY, 0.7f));

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

                if (ball.type == UNIVERSAL && universalIconTexture.id != 0) {
                    Rectangle dest = { ball.position.x - ball.radius, ball.position.y - ball.radius,
                                     ball.radius * 2, ball.radius * 2 };
                    DrawTexturePro(universalIconTexture,
                        { 0, 0, (float)universalIconTexture.width, (float)universalIconTexture.height },
                        dest,
                        { 0, 0 }, 0.0f, WHITE);
                }
                else if (ball.type == BOMB && bombIconTexture.id != 0) {
                    Rectangle dest = { ball.position.x - ball.radius, ball.position.y - ball.radius,
                                     ball.radius * 2, ball.radius * 2 };
                    DrawTexturePro(bombIconTexture,
                        { 0, 0, (float)bombIconTexture.width, (float)bombIconTexture.height },
                        dest,
                        { 0, 0 }, 0.0f, WHITE);
                }
                else if (ball.type == RAINBOW && rainbowIconTexture.id != 0) {
                    Rectangle dest = { ball.position.x - ball.radius, ball.position.y - ball.radius,
                                     ball.radius * 2, ball.radius * 2 };
                    DrawTexturePro(rainbowIconTexture,
                        { 0, 0, (float)rainbowIconTexture.width, (float)rainbowIconTexture.height },
                        dest,
                        { 0, 0 }, 0.0f, WHITE);
                }

                DrawCircleLines(static_cast<int>(ball.position.x), static_cast<int>(ball.position.y),
                    static_cast<int>(ball.radius), Fade(WHITE, 0.3f));

                if (ball.type == BOMB) {
                    DrawCircleLines(static_cast<int>(ball.position.x), static_cast<int>(ball.position.y),
                        static_cast<float>(ball.bombRadius), Fade(RED, 0.2f));
                }
            }
        }

        if (currentBall) {
            DrawCircleV(currentBall->position, ballRadius, currentBall->color);

            if (currentBall->type == UNIVERSAL && universalIconTexture.id != 0) {
                Rectangle dest = { currentBall->position.x - ballRadius, currentBall->position.y - ballRadius,
                                 ballRadius * 2, ballRadius * 2 };
                DrawTexturePro(universalIconTexture,
                    { 0, 0, (float)universalIconTexture.width, (float)universalIconTexture.height },
                    dest,
                    { 0, 0 }, 0.0f, WHITE);
            }
            else if (currentBall->type == BOMB && bombIconTexture.id != 0) {
                Rectangle dest = { currentBall->position.x - ballRadius, currentBall->position.y - ballRadius,
                                 ballRadius * 2, ballRadius * 2 };
                DrawTexturePro(bombIconTexture,
                    { 0, 0, (float)bombIconTexture.width, (float)bombIconTexture.height },
                    dest,
                    { 0, 0 }, 0.0f, WHITE);
            }
            else if (currentBall->type == RAINBOW && rainbowIconTexture.id != 0) {
                Rectangle dest = { currentBall->position.x - ballRadius, currentBall->position.y - ballRadius,
                                 ballRadius * 2, ballRadius * 2 };
                DrawTexturePro(rainbowIconTexture,
                    { 0, 0, (float)rainbowIconTexture.width, (float)rainbowIconTexture.height },
                    dest,
                    { 0, 0 }, 0.0f, WHITE);
            }

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

        if (isLevelMode && currentLevel >= 1 && currentLevel <= static_cast<int>(levels.size())) {
            Level& level = levels[static_cast<size_t>(currentLevel) - 1];

            DrawText(TextFormat("Level: %d - %s", level.levelNumber, level.name.c_str()), 20, 10, 20, WHITE);
            DrawText(TextFormat("Score: %d / %d", score, level.targetScore), 20, 35, 20, WHITE);
        }
        else {
            DrawText(TextFormat("Score: %d", score), 20, 10, 20, WHITE);
            DrawText("Endless Mode", 20, 35, 20, WHITE);
        }

        DrawText(TextFormat("Balls: %zu", balls.size()), screenWidth - 120, 20, 20, WHITE);

        DrawText("LMB - shoot, R - restart, M - menu", 20, screenHeight - 30, 15, LIGHTGRAY);

        if (isLevelMode && currentLevel >= 1 && currentLevel <= static_cast<int>(levels.size())) {
            Level& level = levels[static_cast<size_t>(currentLevel) - 1];

            float progressWidth = 300.0f;
            float progress = static_cast<float>(score) / static_cast<float>(level.targetScore);
            if (progress > 1.0f) progress = 1.0f;

            DrawRectangle(screenWidth / 2 - 150, screenHeight - 40,
                static_cast<int>(progressWidth), 20, GRAY);
            DrawRectangle(screenWidth / 2 - 150, screenHeight - 40,
                static_cast<int>(progress * progressWidth), 20, GREEN);
            DrawRectangleLines(screenWidth / 2 - 150, screenHeight - 40,
                static_cast<int>(progressWidth), 20, WHITE);

            std::string progressText = std::to_string(score) + " / " + std::to_string(level.targetScore);
            DrawText(progressText.c_str(),
                screenWidth / 2 - MeasureText(progressText.c_str(), 15) / 2,
                screenHeight - 38,
                15, WHITE);
        }
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

            if (isLevelMode && currentLevel >= static_cast<int>(levels.size())) {
                DrawText("All levels completed!", screenWidth / 2 - 120, screenHeight / 2 + 40, 25, YELLOW);
            }
            else if (isLevelMode) {
                DrawText(TextFormat("Next level: %d", currentLevel + 1),
                    screenWidth / 2 - 100, screenHeight / 2 + 40, 25, YELLOW);
            }

            DrawText("Press R to continue", screenWidth / 2 - 110, screenHeight / 2 + 80, 20, GREEN);
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
                if (gameState == GAME_OVER || gameState == GAME_WON || gameState == PLAYING) {
                    gameState = MAIN_MENU;
                    restart();
                }
            }

            if (IsKeyPressed(KEY_ESCAPE)) {
                if (gameState == PLAYING) {
                    gameState = MAIN_MENU;
                    restart();
                }
                else if (gameState == LEVEL_SELECT) {
                    gameState = MAIN_MENU;
                }
            }

            update();
            draw();
        }
    }

    void restart() {
        balls.clear();
        particles.clear();
        if (currentBall) {
            delete currentBall;
            currentBall = nullptr;
        }
        score = 0;
        if (gameState == PLAYING) {
            createInitialBalls(isLevelMode);
            createNewBall();
        }
    }
};

int main() {
    BallGame game;
    game.run();
    return 0;
}