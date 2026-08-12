//
//  GameScene.cpp
//  ColorCollapse-desktop
//
//  Created by hoangbui-fe on 12/8/26.
//

#include "GameScene.hpp"
#include "Constants.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <numeric>
#include <string>
#include <vector>

USING_NS_CC;

namespace
{
    constexpr float HALF_PLAYER_W = Constants::PLAYER_WIDTH * 0.5f;
    constexpr float HALF_PLAYER_H = Constants::PLAYER_HEIGHT * 0.5f;

    constexpr float HALF_PLATFORM_W = Constants::PLATFORM_WIDTH * 0.5f;
    constexpr float HALF_PLATFORM_H = Constants::PLATFORM_HEIGHT * 0.5f;

    const Color4F BACKGROUND_COLOR(36.0f / 255.0f, 40.0f / 255.0f, 46.0f / 255.0f, 1.0f);
    const Color4F PLAYER_COLOR(216.0f / 255.0f, 221.0f / 255.0f, 226.0f / 255.0f, 1.0f);
    const Color4B HUD_COLOR(235, 235, 235, 255);

    bool isLeftKey(EventKeyboard::KeyCode key)
    {
        return key == EventKeyboard::KeyCode::KEY_A ||
               key == EventKeyboard::KeyCode::KEY_LEFT_ARROW;
    }

    bool isRightKey(EventKeyboard::KeyCode key)
    {
        return key == EventKeyboard::KeyCode::KEY_D ||
               key == EventKeyboard::KeyCode::KEY_RIGHT_ARROW;
    }
}

Scene* GameScene::createScene()
{
    return GameScene::create();
}

bool GameScene::init()
{
    if (!Scene::init())
        return false;

    const auto seed = static_cast<std::uint32_t>(
        std::chrono::high_resolution_clock::now().time_since_epoch().count());

    _rng.seed(seed);

    createBackground();
    createHUD();
    createPlatforms();
    createPlayer();

    auto keyboardListener = EventListenerKeyboard::create();

    keyboardListener->onKeyPressed =
        CC_CALLBACK_1(GameScene::handleKeyPressed, this);

    keyboardListener->onKeyReleased =
        CC_CALLBACK_1(GameScene::handleKeyReleased, this);

    _eventDispatcher->addEventListenerWithSceneGraphPriority(
        keyboardListener, this);

    scheduleUpdate();

    resetGame();

    return true;
}

void GameScene::createBackground()
{
    auto* background = DrawNode::create();

    background->drawSolidRect(
        Vec2(0.0f, 0.0f),
        Vec2(Constants::DESIGN_WIDTH, Constants::DESIGN_HEIGHT),
        BACKGROUND_COLOR);

    addChild(background, kChildTagBackground);
}

void GameScene::createHUD()
{
    _timeLabel = Label::createWithTTF("TIME 00:00", "fonts/arial.ttf", 22.0f);
    _levelLabel = Label::createWithTTF("LEVEL 1", "fonts/arial.ttf", 22.0f);
    _dangerLabel = Label::createWithTTF("DANGER -", "fonts/arial.ttf", 22.0f);

    _timeLabel->setTextColor(HUD_COLOR);
    _levelLabel->setTextColor(HUD_COLOR);
    _dangerLabel->setTextColor(HUD_COLOR);

    _timeLabel->setAnchorPoint(Vec2(0.0f, 0.5f));
    _levelLabel->setAnchorPoint(Vec2(0.5f, 0.5f));
    _dangerLabel->setAnchorPoint(Vec2(0.0f, 0.5f));

    _timeLabel->setPosition(Vec2(130.0f, Constants::HUD_Y));
    _levelLabel->setPosition(Vec2(640.0f, Constants::HUD_Y));
    _dangerLabel->setPosition(Vec2(875.0f, Constants::HUD_Y));

    addChild(_timeLabel, kChildTagHUD);
    addChild(_levelLabel, kChildTagHUD);
    addChild(_dangerLabel, kChildTagHUD);

    _gameOverLabel = Label::createWithTTF("GAME OVER", "fonts/arial.ttf", 42.0f);
    _survivalLabel = Label::createWithTTF("SURVIVED 00:00", "fonts/arial.ttf", 24.0f);
    _restartLabel = Label::createWithTTF("PRESS R TO RESTART", "fonts/arial.ttf", 22.0f);

    _gameOverLabel->setTextColor(HUD_COLOR);
    _survivalLabel->setTextColor(HUD_COLOR);
    _restartLabel->setTextColor(HUD_COLOR);

    _gameOverLabel->setPosition(Vec2(640.0f, 395.0f));
    _survivalLabel->setPosition(Vec2(640.0f, 345.0f));
    _restartLabel->setPosition(Vec2(640.0f, 305.0f));

    _gameOverLabel->setVisible(false);
    _survivalLabel->setVisible(false);
    _restartLabel->setVisible(false);

    addChild(_gameOverLabel, kChildTagHUDGameOver);
    addChild(_survivalLabel, kChildTagHUDGameOver);
    addChild(_restartLabel, kChildTagHUDGameOver);
}

void GameScene::createPlatforms()
{
    for (int i = 0; i < Constants::PLATFORM_COUNT; ++i)
    {
        auto* node = DrawNode::create();

        const float x =
            16.0f +
            HALF_PLATFORM_W +
            static_cast<float>(i) *
                (Constants::PLATFORM_WIDTH + Constants::PLATFORM_GAP);

        const float y =
            Constants::PLATFORM_BOTTOM_Y + HALF_PLATFORM_H;

        node->setPosition(Vec2(x, y));

        addChild(node, kChildTagPlatforms);

        _platforms[i].node = node;
    }
}

void GameScene::createPlayer()
{
    _playerNode = DrawNode::create();
    addChild(_playerNode, kChildTagPlayer);
    setPlayerVisual();
}

void GameScene::setPlayerVisual()
{
    if (!_playerNode)
        return;

    _playerNode->clear();

    _playerNode->drawSolidRect(
        Vec2(-HALF_PLAYER_W, -HALF_PLAYER_H),
        Vec2(HALF_PLAYER_W, HALF_PLAYER_H),
        PLAYER_COLOR);
}

void GameScene::resetGame()
{
    _moveLeft = false;
    _moveRight = false;

    _grounded = true;
    _gameOver = false;
    _groundPlatform = -1;

    _level = 1;
    _playerVelocityY = 0.0f;
    _coyoteTimer = 0.0f;
    _survivalTime = 0.0f;
    _levelTimer = 0.0f;
    _dangerTimer = 0.0f;
    _dangerInterval = Constants::INITIAL_DANGER_INTERVAL;

    _dangerColor = PlatformColor::Red;
    _hasDanger = false;

    std::array<PlatformColor, Constants::PLATFORM_COUNT> startingColors{};

    for (int i = 0; i < 5; ++i)
    {
        startingColors[i] = PlatformColor::Red;
        startingColors[i + 5] = PlatformColor::Blue;
        startingColors[i + 10] = PlatformColor::Yellow;
    }

    std::shuffle(startingColors.begin(), startingColors.end(), _rng);

    for (int i = 0; i < Constants::PLATFORM_COUNT; ++i)
    {
        auto& platform = _platforms[i];

        platform.color = startingColors[i];
        platform.state = PlatformState::Active;
        platform.stateTime = 0.0f;
        platform.node->setVisible(true);
        platform.node->clear();
        platform.node->drawSolidRect(
            Vec2(-HALF_PLATFORM_W, -HALF_PLATFORM_H),
            Vec2(HALF_PLATFORM_W, HALF_PLATFORM_H),
            colorForPlatform(platform.color));
    }

    const int startPlatform = 3;
    _groundPlatform = startPlatform;

    const float startX =
        16.0f +
        HALF_PLATFORM_W +
        static_cast<float>(startPlatform) *
            (Constants::PLATFORM_WIDTH + Constants::PLATFORM_GAP);

    _playerNode->setPosition(Vec2(
        startX,
        Constants::PLATFORM_BOTTOM_Y +
            Constants::PLATFORM_HEIGHT +
            HALF_PLAYER_H));

    _gameOverLabel->setVisible(false);
    _survivalLabel->setVisible(false);
    _restartLabel->setVisible(false);

    updateHUD();
}

void GameScene::update(float dt)
{
    if (_gameOver)
        return;

    dt = std::min(dt, 0.05f);

    _survivalTime += dt;
    _levelTimer += dt;
    _dangerTimer += dt;

    updateDifficulty();
    updateDanger(dt);
    updatePlatformStates(dt);
    updatePlayer(dt);
    updateHUD();

    const float playerBottom =
        _playerNode->getPositionY() - HALF_PLAYER_H;

    if (playerBottom < 0.0f)
    {
        _gameOver = true;
        showGameOver();
    }
}

void GameScene::updateDifficulty()
{
    while (_levelTimer >= Constants::LEVEL_DURATION)
    {
        _levelTimer -= Constants::LEVEL_DURATION;
        ++_level;

        _dangerInterval *= Constants::DANGER_INTERVAL_SCALE;
    }
}

void GameScene::updateDanger(float dt)
{
    if (_dangerTimer < _dangerInterval)
        return;

    _dangerTimer -= _dangerInterval;
    startDangerEvent();
}

void GameScene::startDangerEvent()
{
    std::array<int, 3> counts{0, 0, 0};

    for (const auto& platform : _platforms)
    {
        if (platform.state == PlatformState::Hidden)
            continue;

        const int index = static_cast<int>(platform.color);
        ++counts[index];
    }

    std::vector<PlatformColor> candidates;

    for (int i = 0; i < 3; ++i)
    {
        if (counts[i] > 0)
            candidates.push_back(static_cast<PlatformColor>(i));
    }

    if (candidates.empty())
        return;

    std::uniform_int_distribution<std::size_t> distribution(
        0, candidates.size() - 1);

    _dangerColor = candidates[distribution(_rng)];
    _hasDanger = true;

    for (auto& platform : _platforms)
    {
        if (platform.state == PlatformState::Active &&
            platform.color == _dangerColor)
        {
            platform.state = PlatformState::Warning;
            platform.stateTime = 0.0f;
            platform.node->setVisible(true);
        }
    }
}

void GameScene::updatePlatformStates(float dt)
{
    for (auto& platform : _platforms)
    {
        if (platform.state == PlatformState::Warning)
        {
            platform.stateTime += dt;

            // Three complete blink cycles in 1.2 seconds:
            // each cycle is 0.4 s, with 0.2 s visible and 0.2 s hidden.
            const float cycleTime =
                std::fmod(platform.stateTime, 0.4f);

            platform.node->setVisible(cycleTime < 0.2f);

            if (platform.stateTime >= Constants::WARNING_DURATION)
            {
                platform.state = PlatformState::Hidden;
                platform.stateTime = 0.0f;
                platform.node->setVisible(false);
            }
        }
        else if (platform.state == PlatformState::Hidden)
        {
            platform.stateTime += dt;

            if (platform.stateTime >= Constants::DISAPPEAR_DURATION)
            {
                recolorPlatform(platform);

                platform.state = PlatformState::Active;
                platform.stateTime = 0.0f;
                platform.node->setVisible(true);

                ensureAllColorsPresent();
            }
        }
    }
}

void GameScene::recolorPlatform(Platform& platform)
{
    std::uniform_int_distribution<int> distribution(0, 2);

    PlatformColor newColor =
        static_cast<PlatformColor>(distribution(_rng));

    if (newColor == platform.color)
    {
        newColor = static_cast<PlatformColor>(
            (static_cast<int>(newColor) + 1 + distribution(_rng) % 2) % 3);
    }

    platform.color = newColor;

    platform.node->clear();
    platform.node->drawSolidRect(
        Vec2(-HALF_PLATFORM_W, -HALF_PLATFORM_H),
        Vec2(HALF_PLATFORM_W, HALF_PLATFORM_H),
        colorForPlatform(platform.color));
}

void GameScene::ensureAllColorsPresent()
{
    std::array<int, 3> counts{0, 0, 0};

    for (const auto& platform : _platforms)
    {
        if (platform.state != PlatformState::Hidden)
            ++counts[static_cast<int>(platform.color)];
    }

    for (int missing = 0; missing < 3; ++missing)
    {
        if (counts[missing] > 0)
            continue;

        int donor = -1;

        for (int i = 0; i < Constants::PLATFORM_COUNT; ++i)
        {
            const auto& platform = _platforms[i];

            if (platform.state != PlatformState::Active &&
                platform.state != PlatformState::Warning)
                continue;

            const int donorColor = static_cast<int>(platform.color);

            if (counts[donorColor] > 1)
            {
                donor = i;
                break;
            }
        }

        if (donor < 0)
            continue;

        auto& platform = _platforms[donor];

        --counts[static_cast<int>(platform.color)];
        platform.color = static_cast<PlatformColor>(missing);
        ++counts[missing];

        platform.node->clear();
        platform.node->drawSolidRect(
            Vec2(-HALF_PLATFORM_W, -HALF_PLATFORM_H),
            Vec2(HALF_PLATFORM_W, HALF_PLATFORM_H),
            colorForPlatform(platform.color));
    }
}

void GameScene::updatePlayer(float dt)
{
    const bool wasGrounded = _grounded;

    float direction = 0.0f;

    if (_moveLeft)
        direction -= 1.0f;

    if (_moveRight)
        direction += 1.0f;

    Vec2 position = _playerNode->getPosition();

    position.x += direction * Constants::PLAYER_SPEED * dt;

    position.x = clampf(
        position.x,
        HALF_PLAYER_W,
        Constants::DESIGN_WIDTH - HALF_PLAYER_W);

    if (_grounded)
    {
        bool stillSupported = false;

        for (int i = 0; i < Constants::PLATFORM_COUNT; ++i)
        {
            const auto& platform = _platforms[i];

            if (platform.state == PlatformState::Hidden)
                continue;

            if (isSupportingPlatform(platform))
            {
                stillSupported = true;
                _groundPlatform = i;

                const float top =
                    Constants::PLATFORM_BOTTOM_Y +
                    Constants::PLATFORM_HEIGHT;

                position.y = top + HALF_PLAYER_H;
                _playerVelocityY = 0.0f;
                break;
            }
        }

        if (!stillSupported)
        {
            _grounded = false;
            _groundPlatform = -1;
        }
    }

    const float previousBottom = position.y - HALF_PLAYER_H;

    if (!_grounded)
    {
        _playerVelocityY -= Constants::GRAVITY * dt;
        position.y += _playerVelocityY * dt;

        const float currentBottom = position.y - HALF_PLAYER_H;

        if (_playerVelocityY <= 0.0f)
            resolveLanding(previousBottom, currentBottom);
    }

    _playerNode->setPosition(position);

    if (wasGrounded && !_grounded)
        _coyoteTimer = Constants::COYOTE_TIME;
    else if (!_grounded)
        _coyoteTimer = std::max(0.0f, _coyoteTimer - dt);
    else
        _coyoteTimer = 0.0f;
}

void GameScene::resolveLanding(float previousBottom, float currentBottom)
{
    int landingPlatform = -1;
    float landingTop = -100000.0f;

    for (int i = 0; i < Constants::PLATFORM_COUNT; ++i)
    {
        const auto& platform = _platforms[i];

        if (platform.state == PlatformState::Hidden)
            continue;

        if (!overlapsHorizontally(platform))
            continue;

        const float top =
            Constants::PLATFORM_BOTTOM_Y +
            Constants::PLATFORM_HEIGHT;

        if (previousBottom + Constants::EPSILON >= top &&
            currentBottom <= top + Constants::EPSILON &&
            top > landingTop)
        {
            landingTop = top;
            landingPlatform = i;
        }
    }

    if (landingPlatform >= 0)
    {
        auto position = _playerNode->getPosition();

        position.y = landingTop + HALF_PLAYER_H;

        _playerNode->setPosition(position);

        _playerVelocityY = 0.0f;
        _grounded = true;
        _groundPlatform = landingPlatform;
        _coyoteTimer = 0.0f;
    }
}

bool GameScene::overlapsHorizontally(const Platform& platform) const
{
    const float playerLeft =
        _playerNode->getPositionX() - HALF_PLAYER_W;

    const float playerRight =
        _playerNode->getPositionX() + HALF_PLAYER_W;

    const float platformLeft =
        platform.node->getPositionX() - HALF_PLATFORM_W;

    const float platformRight =
        platform.node->getPositionX() + HALF_PLATFORM_W;

    return playerRight > platformLeft &&
           playerLeft < platformRight;
}

bool GameScene::isSupportingPlatform(const Platform& platform) const
{
    if (!overlapsHorizontally(platform))
        return false;

    const float playerBottom =
        _playerNode->getPositionY() - HALF_PLAYER_H;

    const float platformTop =
        Constants::PLATFORM_BOTTOM_Y +
        Constants::PLATFORM_HEIGHT;

    return std::fabs(playerBottom - platformTop) <= 2.0f;
}

void GameScene::tryJump()
{
    if (_gameOver)
        return;

    if (_grounded || _coyoteTimer > 0.0f)
    {
        _grounded = false;
        _groundPlatform = -1;
        _coyoteTimer = 0.0f;
        _playerVelocityY = Constants::JUMP_VELOCITY;
    }
}

void GameScene::handleKeyPressed(EventKeyboard::KeyCode keyCode)
{
    if (keyCode == EventKeyboard::KeyCode::KEY_R)
    {
        resetGame();
        return;
    }

    if (_gameOver)
        return;

    if (isLeftKey(keyCode))
        _moveLeft = true;

    if (isRightKey(keyCode))
        _moveRight = true;

    if (keyCode == EventKeyboard::KeyCode::KEY_SPACE)
        tryJump();
}

void GameScene::handleKeyReleased(EventKeyboard::KeyCode keyCode)
{
    if (isLeftKey(keyCode))
        _moveLeft = false;

    if (isRightKey(keyCode))
        _moveRight = false;
}

PlatformColor GameScene::randomColor()
{
    std::uniform_int_distribution<int> distribution(0, 2);
    return static_cast<PlatformColor>(distribution(_rng));
}

Color4F GameScene::colorForPlatform(PlatformColor color) const
{
    switch (color)
    {
        case PlatformColor::Red:
            return Color4F(Color4B(216, 82, 82, 255));

        case PlatformColor::Blue:
            return Color4F(Color4B(67, 132, 215, 255));

        case PlatformColor::Yellow:
            return Color4F(Color4B(228, 194, 68, 255));
    }

    return Color4F::WHITE;
}

const char* GameScene::colorName(PlatformColor color) const
{
    switch (color)
    {
        case PlatformColor::Red:
            return "RED";

        case PlatformColor::Blue:
            return "BLUE";

        case PlatformColor::Yellow:
            return "YELLOW";
    }

    return "-";
}

void GameScene::updateHUD()
{
    char timeBuffer[32];
    formatTime(_survivalTime, timeBuffer, sizeof(timeBuffer));

    _timeLabel->setString(
        std::string("TIME ") + timeBuffer);

    _levelLabel->setString(
        std::string("LEVEL ") + std::to_string(_level));

    if (_dangerTimer < 0.01f && !_gameOver)
    {
        // Keep the last selected danger color visible.
    }

    _dangerLabel->setString(
        _hasDanger
            ? std::string("DANGER ") + colorName(_dangerColor)
            : "DANGER -");
}

void GameScene::showGameOver()
{
    char timeBuffer[32];
    formatTime(_survivalTime, timeBuffer, sizeof(timeBuffer));

    _gameOverLabel->setVisible(true);
    _survivalLabel->setVisible(true);
    _restartLabel->setVisible(true);

    _survivalLabel->setString(
        std::string("SURVIVED ") + timeBuffer);
}

void GameScene::formatTime(
    float seconds,
    char* buffer,
    std::size_t bufferSize) const
{
    const int totalSeconds =
        static_cast<int>(std::floor(seconds));

    const int minutes = totalSeconds / 60;
    const int secs = totalSeconds % 60;

    std::snprintf(
        buffer,
        bufferSize,
        "%02d:%02d",
        minutes,
        secs);
}
