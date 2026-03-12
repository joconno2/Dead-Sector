#include "PhysicsSystem.hpp"
#include "entities/Entity.hpp"
#include <cmath>

PhysicsSystem::PhysicsSystem(float worldW, float worldH)
    : m_worldW(worldW), m_worldH(worldH)
{}

void PhysicsSystem::integrate(Entity& e, float dt) {
    e.pos += e.vel * dt;
}

void PhysicsSystem::wrapEdges(Entity& e) {
    float r = e.radius;
    if (e.pos.x < -r)          e.pos.x += m_worldW + 2.f * r;
    if (e.pos.x > m_worldW + r) e.pos.x -= m_worldW + 2.f * r;
    if (e.pos.y < -r)          e.pos.y += m_worldH + 2.f * r;
    if (e.pos.y > m_worldH + r) e.pos.y -= m_worldH + 2.f * r;
}

void PhysicsSystem::update(float dt, std::vector<Entity*> entities) {
    for (Entity* e : entities) {
        if (!e || !e->alive) continue;
        integrate(*e, dt);
        if (!e->noWrap) wrapEdges(*e);
        e->update(dt);
        // transformVerts is called via the public method exposed on Entity
        // We call it through the protected helper by having Entity::update
        // subclasses call it, or we call it here via a friend/virtual approach.
        // Simplest: add a public refreshVerts() to Entity.
        e->refreshVerts();
    }
}
