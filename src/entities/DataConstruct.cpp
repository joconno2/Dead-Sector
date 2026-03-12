#include "DataConstruct.hpp"
#include "core/Constants.hpp"
#include <random>
#include <cmath>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static float randomFloat(std::mt19937& rng, float lo, float hi) {
    return std::uniform_real_distribution<float>(lo, hi)(rng);
}

// ---------------------------------------------------------------------------
// Static factory
// ---------------------------------------------------------------------------

std::unique_ptr<DataConstruct> DataConstruct::createLarge(Vec2 pos, Vec2 vel) {
    std::mt19937 rng(static_cast<unsigned>(pos.x * 1009.f + pos.y * 2003.f));
    float ang = randomFloat(rng, 0.f, 6.2831853f);
    return std::make_unique<DataConstruct>(pos, vel, ConstructSize::Large, ang);
}

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

DataConstruct::DataConstruct(Vec2 pos_, Vec2 vel_, ConstructSize size_, float angle_)
    : size(size_)
{
    pos   = pos_;
    vel   = vel_;
    angle = angle_;
    type  = EntityType::DataConstruct;

    switch (size) {
        case ConstructSize::Large:  radius = Constants::CONSTRUCT_RADIUS_LARGE;  break;
        case ConstructSize::Medium: radius = Constants::CONSTRUCT_RADIUS_MEDIUM; break;
        case ConstructSize::Small:  radius = Constants::CONSTRUCT_RADIUS_SMALL;  break;
    }

    unsigned seed = static_cast<unsigned>(pos_.x * 1009.f + pos_.y * 2003.f + (int)size_ * 7);
    m_localVerts  = generateVerts(size, seed);

    std::mt19937 rng(seed ^ 0xDEAD);
    m_rotSpeed = randomFloat(rng, Constants::CONSTRUCT_ROT_MIN, Constants::CONSTRUCT_ROT_MAX);
    if (rng() & 1) m_rotSpeed = -m_rotSpeed;

    transformVerts();
}

// ---------------------------------------------------------------------------
// Vertex generation — randomised convex-ish polygon in local space
// ---------------------------------------------------------------------------

std::vector<Vec2> DataConstruct::generateVerts(ConstructSize sz, unsigned seed) {
    std::mt19937 rng(seed);

    float baseR;
    int   numVerts;
    switch (sz) {
        case ConstructSize::Large:  baseR = 45.f; numVerts = 7; break;
        case ConstructSize::Medium: baseR = 25.f; numVerts = 6; break;
        case ConstructSize::Small:  baseR = 12.f; numVerts = 5; break;
        default:                    baseR = 25.f; numVerts = 6; break;
    }

    std::vector<Vec2> verts;
    verts.reserve(numVerts);

    float angleStep = (2.f * 3.14159265f) / (float)numVerts;
    for (int i = 0; i < numVerts; ++i) {
        float a = angleStep * (float)i + randomFloat(rng, -0.25f, 0.25f);
        float r = randomFloat(rng, baseR * 0.55f, baseR * 1.0f);
        verts.push_back({std::cos(a) * r, std::sin(a) * r});
    }
    return verts;
}

// ---------------------------------------------------------------------------
// Update
// ---------------------------------------------------------------------------

void DataConstruct::update(float dt) {
    angle += m_rotSpeed * dt;
}

// ---------------------------------------------------------------------------
// Fragmentation
// ---------------------------------------------------------------------------

int DataConstruct::scoreValue() const {
    switch (size) {
        case ConstructSize::Large:  return Constants::SCORE_LARGE;
        case ConstructSize::Medium: return Constants::SCORE_MEDIUM;
        case ConstructSize::Small:  return Constants::SCORE_SMALL;
        default:                    return 0;
    }
}

std::vector<std::unique_ptr<DataConstruct>> DataConstruct::fragment() {
    std::vector<std::unique_ptr<DataConstruct>> result;

    if (size == ConstructSize::Small) return result; // terminal

    ConstructSize childSize  = (size == ConstructSize::Large)
                             ? ConstructSize::Medium
                             : ConstructSize::Small;
    int childCount           = (size == ConstructSize::Large) ? 3 : 2;
    float childSpeed         = (childSize == ConstructSize::Medium)
                             ? Constants::CONSTRUCT_SPEED_MEDIUM
                             : Constants::CONSTRUCT_SPEED_SMALL;

    std::mt19937 rng(static_cast<unsigned>(pos.x * 73.f + pos.y * 137.f));
    std::uniform_real_distribution<float> angleDist(0.f, 6.2831853f);

    for (int i = 0; i < childCount; ++i) {
        float a   = angleDist(rng);
        Vec2  dir = {std::cos(a), std::sin(a)};
        Vec2  cvel = vel * 0.6f + dir * childSpeed;
        Vec2  spawnPos = pos + dir * (radius * 0.5f);
        float childAngle = angleDist(rng);

        result.push_back(std::make_unique<DataConstruct>(
            spawnPos, cvel, childSize, childAngle));
    }
    return result;
}
