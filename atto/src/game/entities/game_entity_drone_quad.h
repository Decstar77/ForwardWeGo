#pragma once

#include "game_entity.h"
#include <vector>

namespace atto {
    enum class DroneAIState {
        Wander,
        Chase,
        Attack
    };

    class Entity_DroneQuad : public Entity {
    public:
        Entity_DroneQuad();

        void OnSpawn() override;
        void OnUpdate( f32 dt ) override;
        void OnRender( Renderer & renderer ) override;
        void OnDespawn() override;

        AlignedBox GetBounds() const override;
        bool RayTest( const Vec3 & start, const Vec3 & dir, f32 & dist ) const override;
        void DebugDrawBounds( Renderer & renderer ) override;
        TakeDamageResult TakeDamage( i32 damage ) override;

        void MoveTo( const Vec3 & target );

        void Serialize( Serializer & serializer ) override;

    private:
        void BuildSmoothedPath( const std::vector<i32> & nodePath );
        void PickNewWanderTarget();
        void PathToward( const Vec3 & target );
        bool CanSeePlayer() const;
        bool HasLineOfSightTo( const Vec3 & target ) const;

        const StaticModel * model = nullptr;
        i32 health = 100;

        // Logical position (hover bob is added on top for rendering)
        Vec3 basePosition = Vec3( 0.0f );
        Vec3 velocity = Vec3( 0.0f );
        Vec3 moveTarget = Vec3( 0.0f );
        bool hasTarget = false;

        // Accumulated time for periodic hover/wobble
        f32 hoverTime = 0.0f;

        // Smoothed orientation angles (radians)
        f32 smoothYaw = 0.0f;
        f32 smoothPitch = 0.0f;
        f32 smoothRoll = 0.0f;

        // Smoothed path following
        std::vector<Vec3> smoothedPath;
        i32 smoothedPathIndex = 0;

        // AI state
        DroneAIState aiState = DroneAIState::Wander;
        f32 attackCooldown = 0.0f;
        f32 lostSightTimer = 0.0f;
        f32 rePathTimer = 0.0f;
    };
}
