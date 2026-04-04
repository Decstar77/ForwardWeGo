#pragma once

#include "game_entity.h"

namespace atto {

    enum class AgentState {
        Idle,
        Wonder,
        Chase,
        Attack
    };

    struct AgentConfig {
        f32 moveSpeed        = 2.8f;
        f32 arrivalDist      = 0.65f;
        f32 yawSpeed         = 6.0f;
        f32 detectionRange   = 50.0f;
        f32 attackRange      = 2.0f;
        f32 attackRangeHyst  = 2.0f;
        f32 attackRate       = 1.0f;
        f32 lostSightTime    = 3.0f;
        f32 rePathInterval   = 1.5f;
        f32 avoidRadius      = 0.75f;
        f32 avoidStrength    = 3.0f;
        i32 maxHealth        = 100;
    };

    // Base class for ground AI agents (pathfinding, state machine, movement, health).
    // Derived classes must implement OnAgentAttack() to define attack behaviour,
    // and call AgentOnSpawn / AgentOnUpdate / AgentOnRender from their overrides.
    class Entity_Agent : public Entity {
    public:
        void OnRender( Renderer &renderer ) override;

        TakeDamageResult TakeDamage( i32 damage ) override;

        AlignedBox GetBounds() const override;
        Box        GetCollider() const override;
        bool       RayTest( const Vec3 &start, const Vec3 &dir, f32 &dist ) const override;
        void       Serialize( Serializer &serializer ) override;

        void DebugDrawBounds( Renderer &renderer ) override;
        void DebugDrawCollider( Renderer &renderer ) override;
        void DebugDrawPath( Renderer &renderer );

        void MoveTo( Vec3 target );

    protected:
        // Called once per attack cooldown tick while in Attack state.
        virtual void OnAgentAttack() = 0;

        // Called when health reaches 0. Default drops a coin (10% health pickup).
        virtual void OnAgentDeath();

        // Helpers for derived OnUpdate / OnSpawn
        void AgentOnUpdate( f32 dt );

        void PickPathTo( const Vec3 &dest );
        bool HasLineOfSightTo( const Vec3 &target ) const;
        bool CanSeePlayer() const;
        Vec3 Avoidance( Vec3 currentDirection, f32 dt );

        AgentConfig        config;
        const StaticModel *model       = nullptr;
        i32                health      = 100;
        AgentState         state       = AgentState::Idle;
        bool               hasTarget   = false;
        Vec3               agentTarget = Vec3( 0.0f );
        std::vector<Vec3>  path        = {};
        i32                pathIndex   = 0;
        f32                facingYaw   = 0.0f;
        f32                thinkTimer  = 1.0f;
        f32                lostSightTimer = 0.0f;
        f32                rePathTimer    = 0.0f;
        f32                attackCooldown = 0.0f;
        f32                drawRed        = 0.0f;

        SoundCollection     sndWalk;
        SoundCollection     sndDeath;
        f32                 walkSoundTimer = 0.0f;
    };
}
