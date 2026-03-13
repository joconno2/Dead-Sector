#pragma once
#include "math/Vec2.hpp"

// A collectible data fragment dropped by ICE on death during Extract objectives.
// Avatar overlaps it to collect; auto-despawns after lifetime seconds.
struct DataPacket {
    Vec2  pos;
    Vec2  vel;
    float radius   = 10.f;
    float lifetime = 14.f;  // seconds until auto-despawn
    bool  alive    = true;
};
