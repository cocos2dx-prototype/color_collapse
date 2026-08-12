//
//  Constants.h
//  ColorCollapse-desktop
//
//  Created by hoangbui-fe on 12/8/26.
//

#ifndef Constants_h
#define Constants_h

namespace Constants
{
    constexpr float DESIGN_WIDTH = 1280.0f;
    constexpr float DESIGN_HEIGHT = 720.0f;

    constexpr float PLATFORM_WIDTH = 72.0f;
    constexpr float PLATFORM_HEIGHT = 24.0f;
    constexpr float PLATFORM_GAP = 12.0f;
    constexpr int PLATFORM_COUNT = 15;
    constexpr float PLATFORM_BOTTOM_Y = 80.0f;

    constexpr float PLAYER_WIDTH = 32.0f;
    constexpr float PLAYER_HEIGHT = 48.0f;
    constexpr float PLAYER_SPEED = 320.0f;

    constexpr float JUMP_VELOCITY = 650.0f;
    constexpr float GRAVITY = 1800.0f;
    constexpr float COYOTE_TIME = 0.12f;

    constexpr float INITIAL_DANGER_INTERVAL = 4.0f;
    constexpr float WARNING_DURATION = 1.2f;
    constexpr float DISAPPEAR_DURATION = 1.5f;

    constexpr float LEVEL_DURATION = 15.0f;
    constexpr float DANGER_INTERVAL_SCALE = 0.90f;

    constexpr float HUD_Y = 675.0f;

    constexpr float EPSILON = 0.01f;
}

#endif /* Constants_h */
