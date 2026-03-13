#pragma once
#include <functional>
#include <memory>
#include <vector>

class Avatar;
class Projectile;
class EnemyProjectile;
class HunterICE;
class SentryICE;
class SpawnerICE;
class PhantomICE;
class LeechICE;
class MirrorICE;
class Entity;

struct CollisionEvent {
    enum class Type {
        ProjectileHitICE,
        AvatarHitICE,
        EnemyProjectileHitAvatar,
        ProjectileHitAvatar,   // friendly fire
    };
    Type             type;
    Projectile*      projectile = nullptr;
    Entity*          iceEntity  = nullptr;   // HunterICE / SentryICE / SpawnerICE
    EnemyProjectile* enemyProj  = nullptr;
    Avatar*          avatar     = nullptr;
};

class CollisionSystem {
public:
    using EventCallback = std::function<void(const CollisionEvent&)>;

    void setCallback(EventCallback cb) { m_callback = cb; }

    void update(
        Avatar*                                        avatar,
        std::vector<std::unique_ptr<Projectile>>&      projectiles,
        std::vector<std::unique_ptr<HunterICE>>&       hunters,
        std::vector<std::unique_ptr<SentryICE>>&       sentries,
        std::vector<std::unique_ptr<SpawnerICE>>&      spawners,
        std::vector<std::unique_ptr<PhantomICE>>&      phantoms,
        std::vector<std::unique_ptr<LeechICE>>&        leeches,
        std::vector<std::unique_ptr<MirrorICE>>&       mirrors,
        std::vector<std::unique_ptr<EnemyProjectile>>& enemyProjectiles
    );

private:
    EventCallback m_callback;

    static bool circlesOverlap(const Entity& a, const Entity& b);

    template<typename ICEType>
    void checkProjectilesVsICE(
        std::vector<std::unique_ptr<Projectile>>& projectiles,
        std::vector<std::unique_ptr<ICEType>>&    iceVec);

    template<typename ICEType>
    bool checkAvatarVsICE(
        Avatar& avatar,
        std::vector<std::unique_ptr<ICEType>>& iceVec);
};
