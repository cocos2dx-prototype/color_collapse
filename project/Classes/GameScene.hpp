//
//  GameScene.hpp
//  ColorCollapse-desktop
//
//  Created by hoangbui-fe on 12/8/26.
//

#ifndef GameScene_hpp
#define GameScene_hpp

#include "cocos2d.h"
#include <array>
#include <random>

enum class PlatformColor
{
    Red,
    Blue,
    Yellow
};

enum class PlatformState
{
    Active,
    Warning,
    Hidden
};

struct Platform
{
    cocos2d::DrawNode* node = nullptr;
    PlatformColor color = PlatformColor::Red;
    PlatformState state = PlatformState::Active;
    float stateTime = 0.0f;
};

class GameScene final : public cocos2d::Scene
{
public:
    enum ChildTag {
        None = 0,
        kChildTagBackground,
        kChildTagHUD,
        kChildTagPlatforms,
        kChildTagPlayer,
        kChildTagHUDGameOver
    };
    
public:
    static cocos2d::Scene* createScene();
    
private:
    CREATE_FUNC(GameScene);
    virtual bool init() override;
    virtual void onEnter() override;
    virtual void onExit() override;

private:
    cocos2d::DrawNode* _playerNode = nullptr;

    cocos2d::Label* _timeLabel = nullptr;
    cocos2d::Label* _levelLabel = nullptr;
    cocos2d::Label* _dangerLabel = nullptr;

    cocos2d::Label* _gameOverLabel = nullptr;
    cocos2d::Label* _survivalLabel = nullptr;
    cocos2d::Label* _restartLabel = nullptr;

    std::array<Platform, 15> _platforms{};

    std::mt19937 _rng;

    bool _moveLeft = false;
    bool _moveRight = false;
    bool _grounded = true;
    bool _gameOver = false;
    bool _hasDanger = false;

    int _groundPlatform = -1;
    int _level = 1;

    float _playerVelocityY = 0.0f;
    float _coyoteTimer = 0.0f;
    float _survivalTime = 0.0f;
    float _levelTimer = 0.0f;
    float _dangerTimer = 0.0f;
    float _dangerInterval = 4.0f;

    PlatformColor _dangerColor = PlatformColor::Red;
    
private:
    void resetGame();
    void createBackground();
    void createHUD();
    void createPlatforms();
    void createPlayer();

    void update(float dt) override;
    void updateDifficulty();
    void updateDanger(float dt);
    void startDangerEvent();
    void updatePlatformStates(float dt);
    void recolorPlatform(Platform& platform);
    void ensureAllColorsPresent();

    void updatePlayer(float dt);
    void resolveLanding(float previousBottom, float currentBottom);
    bool overlapsHorizontally(const Platform& platform) const;
    bool isSupportingPlatform(const Platform& platform) const;

    void tryJump();
    void handleKeyPressed(cocos2d::EventKeyboard::KeyCode keyCode);
    void handleKeyReleased(cocos2d::EventKeyboard::KeyCode keyCode);

    PlatformColor randomColor();
    cocos2d::Color4F colorForPlatform(PlatformColor color) const;
    const char* colorName(PlatformColor color) const;

    void setPlayerVisual();
    void updateHUD();
    void showGameOver();
    void formatTime(float seconds, char* buffer, std::size_t bufferSize) const;
};


#endif /* GameScene_hpp */
