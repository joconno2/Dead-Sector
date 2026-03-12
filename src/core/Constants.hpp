#pragma once
#include <cstdint>

namespace Constants {

    // Window
    constexpr int   SCREEN_W  = 1280;
    constexpr int   SCREEN_H  = 720;
    constexpr float SCREEN_WF = 1280.f;
    constexpr float SCREEN_HF = 720.f;

    // Avatar physics
    constexpr float ROT_SPEED      = 3.6f;   // radians/sec
    constexpr float THRUST_FORCE   = 220.f;  // px/s²
    constexpr float MAX_SPEED      = 380.f;  // px/s hard cap
    constexpr float DRAG           = 0.0f;   // 0 = pure Asteroids inertia
    constexpr float RECOIL_IMPULSE = 18.f;   // px/s backward per shot

    // Avatar geometry
    constexpr float AVATAR_RADIUS = 16.f;

    // Projectile
    constexpr float PROJ_SPEED    = 620.f;   // px/s (world-relative)
    constexpr float PROJ_LIFETIME = 1.6f;    // seconds

    // DataConstruct radii by size
    constexpr float CONSTRUCT_RADIUS_LARGE  = 45.f;
    constexpr float CONSTRUCT_RADIUS_MEDIUM = 25.f;
    constexpr float CONSTRUCT_RADIUS_SMALL  = 12.f;

    // DataConstruct drift speeds
    constexpr float CONSTRUCT_SPEED_LARGE  = 40.f;
    constexpr float CONSTRUCT_SPEED_MEDIUM = 75.f;
    constexpr float CONSTRUCT_SPEED_SMALL  = 130.f;

    // DataConstruct rotation speeds (radians/sec)
    constexpr float CONSTRUCT_ROT_MIN = 0.3f;
    constexpr float CONSTRUCT_ROT_MAX = 1.2f;

    // Ammo
    constexpr int STARTING_AMMO = 30;

    // Score
    constexpr int SCORE_LARGE  = 20;
    constexpr int SCORE_MEDIUM = 50;
    constexpr int SCORE_SMALL  = 100;

    // (DataConstruct initial spawn removed — combat uses pure ICE escalation)

    // Palette (R, G, B)
    constexpr uint8_t COL_AVATAR_R     = 80,  COL_AVATAR_G     = 220, COL_AVATAR_B     = 255;
    constexpr uint8_t COL_CONSTRUCT_R  = 255, COL_CONSTRUCT_G  = 60,  COL_CONSTRUCT_B  = 120;
    constexpr uint8_t COL_PROJ_R       = 255, COL_PROJ_G       = 255, COL_PROJ_B       = 100;
    constexpr uint8_t COL_HUD_R        = 80,  COL_HUD_G        = 255, COL_HUD_B        = 150;

    // Font
    constexpr const char* FONT_PATH = "assets/fonts/ShareTechMono-Regular.ttf";
    constexpr int         FONT_SIZE = 20;

    // Trace system
    constexpr float TRACE_TICK_RATE   = 3.0f;   // %/sec baseline tick rate
    constexpr float TRACE_HIT_PENALTY = 8.0f;   // % added when avatar is hit
    constexpr float TRACE_ALARM_ADD   = 15.0f;  // % added on sentry alarm
    constexpr float TRACE_THRESH_1    = 25.0f;
    constexpr float TRACE_THRESH_2    = 50.0f;
    constexpr float TRACE_THRESH_3    = 75.0f;
    constexpr float TRACE_THRESH_4    = 100.0f;

    // Spawn manager
    constexpr int   SPAWN_MAX_ENTITIES  = 16;    // hard cap on live ICE count
    constexpr float SPAWN_INTERVAL_BASE = 5.0f;  // seconds between spawns at low trace
    constexpr float SPAWN_INTERVAL_MIN  = 1.2f;  // fastest possible spawn interval

    // HunterICE
    constexpr float HUNTER_MAX_SPEED = 200.f;
    constexpr float HUNTER_ACCEL     = 280.f;
    constexpr float HUNTER_RADIUS    = 12.f;
    constexpr int   SCORE_HUNTER     = 75;

    // SentryICE
    constexpr float SENTRY_RADIUS    = 18.f;
    constexpr float SENTRY_ROT_SPEED = 1.8f;
    constexpr float SENTRY_FIRE_RATE = 3.8f;  // 3-way burst, so longer interval
    constexpr float SENTRY_PROJ_SPEED = 220.f;
    constexpr int   SCORE_SENTRY     = 150;

    // SpawnerICE
    constexpr float SPAWNER_RADIUS       = 22.f;
    constexpr float SPAWNER_SPAWN_RATE   = 5.5f;
    constexpr int   SPAWNER_MAX_CHILDREN = 4;
    constexpr int   SCORE_SPAWNER        = 200;

    // EnemyProjectile
    constexpr float ENEMY_PROJ_SPEED    = 220.f;
    constexpr float ENEMY_PROJ_LIFETIME = 1.4f;
    constexpr float ENEMY_PROJ_RADIUS   = 3.f;

    // ICE colors
    constexpr uint8_t COL_HUNTER_R  = 255, COL_HUNTER_G  = 40,  COL_HUNTER_B  = 60;
    constexpr uint8_t COL_SENTRY_R  = 255, COL_SENTRY_G  = 160, COL_SENTRY_B  = 20;
    constexpr uint8_t COL_SPAWNER_R = 200, COL_SPAWNER_G = 60,  COL_SPAWNER_B = 255;
    constexpr uint8_t COL_EPROJ_R   = 255, COL_EPROJ_G   = 140, COL_EPROJ_B   = 30;
}
