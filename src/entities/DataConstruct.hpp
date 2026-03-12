#pragma once
#include "Entity.hpp"
#include <vector>
#include <memory>

enum class ConstructSize { Large = 3, Medium = 2, Small = 1 };

class DataConstruct : public Entity {
public:
    static std::unique_ptr<DataConstruct> createLarge(Vec2 pos, Vec2 vel);

    DataConstruct(Vec2 pos, Vec2 vel, ConstructSize size, float angle);

    void update(float dt) override;

    // Returns 0–3 child constructs. Caller appends them to the world.
    std::vector<std::unique_ptr<DataConstruct>> fragment();

    int scoreValue() const;

    ConstructSize size;

private:
    float m_rotSpeed = 0.f;

    static std::vector<Vec2> generateVerts(ConstructSize sz, unsigned seed);
};
