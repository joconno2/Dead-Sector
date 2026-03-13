#include "CollisionSystem.hpp"
#include "entities/Avatar.hpp"
#include "entities/Projectile.hpp"
#include "entities/EnemyProjectile.hpp"
#include "entities/HunterICE.hpp"
#include "entities/SentryICE.hpp"
#include "entities/SpawnerICE.hpp"
#include "entities/PhantomICE.hpp"
#include "entities/LeechICE.hpp"
#include "entities/MirrorICE.hpp"
#include "core/Constants.hpp"

bool CollisionSystem::circlesOverlap(const Entity& a, const Entity& b) {
    float dx   = a.pos.x - b.pos.x;
    float dy   = a.pos.y - b.pos.y;
    float rSum = a.radius + b.radius;
    return (dx * dx + dy * dy) < (rSum * rSum);
}

// ---------------------------------------------------------------------------
// Template helpers
// ---------------------------------------------------------------------------

template<typename ICEType>
void CollisionSystem::checkProjectilesVsICE(
    std::vector<std::unique_ptr<Projectile>>& projectiles,
    std::vector<std::unique_ptr<ICEType>>&    iceVec)
{
    for (auto& proj : projectiles) {
        if (!proj->alive) continue;
        for (auto& ice : iceVec) {
            if (!ice->alive) continue;
            if (circlesOverlap(*proj, *ice)) {
                CollisionEvent ev;
                ev.type       = CollisionEvent::Type::ProjectileHitICE;
                ev.projectile = proj.get();
                ev.iceEntity  = ice.get();
                m_callback(ev);
            }
        }
    }
}

template<typename ICEType>
bool CollisionSystem::checkAvatarVsICE(
    Avatar& avatar,
    std::vector<std::unique_ptr<ICEType>>& iceVec)
{
    for (auto& ice : iceVec) {
        if (!ice->alive) continue;
        if (circlesOverlap(avatar, *ice)) {
            CollisionEvent ev;
            ev.type      = CollisionEvent::Type::AvatarHitICE;
            ev.iceEntity = ice.get();
            ev.avatar    = &avatar;
            m_callback(ev);
            return true;
        }
    }
    return false;
}

// Explicit instantiations
template void CollisionSystem::checkProjectilesVsICE<HunterICE>(
    std::vector<std::unique_ptr<Projectile>>&, std::vector<std::unique_ptr<HunterICE>>&);
template void CollisionSystem::checkProjectilesVsICE<SentryICE>(
    std::vector<std::unique_ptr<Projectile>>&, std::vector<std::unique_ptr<SentryICE>>&);
template void CollisionSystem::checkProjectilesVsICE<SpawnerICE>(
    std::vector<std::unique_ptr<Projectile>>&, std::vector<std::unique_ptr<SpawnerICE>>&);
template void CollisionSystem::checkProjectilesVsICE<PhantomICE>(
    std::vector<std::unique_ptr<Projectile>>&, std::vector<std::unique_ptr<PhantomICE>>&);

template bool CollisionSystem::checkAvatarVsICE<HunterICE>(
    Avatar&, std::vector<std::unique_ptr<HunterICE>>&);
template bool CollisionSystem::checkAvatarVsICE<SentryICE>(
    Avatar&, std::vector<std::unique_ptr<SentryICE>>&);
template bool CollisionSystem::checkAvatarVsICE<SpawnerICE>(
    Avatar&, std::vector<std::unique_ptr<SpawnerICE>>&);
template bool CollisionSystem::checkAvatarVsICE<PhantomICE>(
    Avatar&, std::vector<std::unique_ptr<PhantomICE>>&);
template void CollisionSystem::checkProjectilesVsICE<LeechICE>(
    std::vector<std::unique_ptr<Projectile>>&, std::vector<std::unique_ptr<LeechICE>>&);
template bool CollisionSystem::checkAvatarVsICE<LeechICE>(
    Avatar&, std::vector<std::unique_ptr<LeechICE>>&);
template bool CollisionSystem::checkAvatarVsICE<MirrorICE>(
    Avatar&, std::vector<std::unique_ptr<MirrorICE>>&);

// ---------------------------------------------------------------------------
// Main update
// ---------------------------------------------------------------------------

void CollisionSystem::update(
    Avatar*                                        avatar,
    std::vector<std::unique_ptr<Projectile>>&      projectiles,
    std::vector<std::unique_ptr<HunterICE>>&       hunters,
    std::vector<std::unique_ptr<SentryICE>>&       sentries,
    std::vector<std::unique_ptr<SpawnerICE>>&      spawners,
    std::vector<std::unique_ptr<PhantomICE>>&      phantoms,
    std::vector<std::unique_ptr<LeechICE>>&        leeches,
    std::vector<std::unique_ptr<MirrorICE>>&       mirrors,
    std::vector<std::unique_ptr<EnemyProjectile>>& enemyProjectiles)
{
    if (!m_callback) return;

    // Player projectiles vs ICE (MirrorICE handled specially in CombatScene)
    checkProjectilesVsICE(projectiles, hunters);
    checkProjectilesVsICE(projectiles, sentries);
    checkProjectilesVsICE(projectiles, spawners);
    checkProjectilesVsICE(projectiles, phantoms);
    checkProjectilesVsICE(projectiles, leeches);

    // Avatar vs ICE — one hit ends the run
    if (!avatar || !avatar->alive) return;
    if (checkAvatarVsICE(*avatar, hunters))  return;
    if (checkAvatarVsICE(*avatar, sentries)) return;
    if (checkAvatarVsICE(*avatar, spawners)) return;
    if (checkAvatarVsICE(*avatar, phantoms)) return;
    if (checkAvatarVsICE(*avatar, leeches))  return;
    if (checkAvatarVsICE(*avatar, mirrors))  return;

    // Player projectiles vs avatar (friendly fire)
    // Skip projectiles younger than 150ms (just fired, still overlapping spawn point)
    for (auto& proj : projectiles) {
        if (!proj->alive || !avatar->alive) continue;
        if (proj->lifetime > Constants::PROJ_LIFETIME - 0.15f) continue;
        if (circlesOverlap(*proj, *avatar)) {
            CollisionEvent ev;
            ev.type      = CollisionEvent::Type::ProjectileHitAvatar;
            ev.projectile = proj.get();
            ev.avatar    = avatar;
            m_callback(ev);
            return;
        }
    }

    // Enemy projectiles vs avatar
    for (auto& ep : enemyProjectiles) {
        if (!ep->alive) continue;
        if (circlesOverlap(*ep, *avatar)) {
            CollisionEvent ev;
            ev.type      = CollisionEvent::Type::EnemyProjectileHitAvatar;
            ev.enemyProj = ep.get();
            ev.avatar    = avatar;
            m_callback(ev);
            return;
        }
    }
}
