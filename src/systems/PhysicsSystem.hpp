#pragma once
#include <vector>

class Entity;

class PhysicsSystem {
public:
    PhysicsSystem(float worldW, float worldH);

    // Integrate, wrap, and refresh world verts for all entities
    void update(float dt, std::vector<Entity*> entities);

private:
    float m_worldW;
    float m_worldH;

    void integrate(Entity& e, float dt);
    void wrapEdges(Entity& e);
};
