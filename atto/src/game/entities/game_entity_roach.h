#pragma once

#include "game_entity.h"
namespace atto {

    enum class RoachState {
        Idle,
        Wonder,
        Chase,
        Attack
    };

    class Entity_Roach : public Entity {
    public:
        Entity_Roach();

        void OnSpawn() override;
        void OnUpdate( f32 dt ) override;
        void OnRender( Renderer &renderer ) override;
        void OnDespawn() override;

        AlignedBox GetBounds() const override;
        Box GetCollider() const override;
        bool RayTest( const Vec3 &start, const Vec3 &dir, f32 &dist ) const override;
        void Serialize( Serializer &serializer ) override;

        TakeDamageResult TakeDamage( i32 damage ) override;

        void DebugDrawBounds( Renderer &renderer ) override;
        void DebugDrawCollider( Renderer & renderer ) override;
        void DebugDrawPath( Renderer & renderer );

        void MoveTo( Vec3 target );

    private:
        void PickPathTo( const Vec3 & dest );
        bool HasLineOfSightTo( const Vec3 & target ) const;
        bool CanSeePlayer() const;

        Vec3 Avoidance( Vec3 currentDirection, f32 dt );

        const StaticModel * model = nullptr;
        i32 health = 100;
        RoachState state = RoachState::Idle;
        bool hasTarget = false;
        Vec3 target = Vec3( 0.0f, 0.0f, 0.0f );
        std::vector<Vec3> path = {};
        i32 pathIndex = 0;
        f32 facingYaw = 0.0f;
        f32 thinkTimer = 1.0f;
        f32 lostSightTimer = 0.0f;
        f32 rePathTimer = 0.0f;
        f32 attackCooldown = 0.0f;
        f32 drawRed = 0.0f;
    };
}


