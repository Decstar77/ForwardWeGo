//
// Created by porte on 3/22/2026.
//

#include "game_entity_roach.h"

#include "game/game_map.h"

namespace atto {
    ATTO_REGISTER_CLASS(  Entity, Entity_Roach, EntityType::Roach )

    Entity_Roach::Entity_Roach() {
        type = EntityType::Roach;
    }

    void Entity_Roach::OnSpawn() {
        model = Engine::Get().GetRenderer().GetOrLoadStaticModel( "assets/models/sm/SM_Prop_DeadZub_01.obj" );
    }

    static constexpr f32 RoachMoveSpeed       = 1.8f;  // m/s
    static constexpr f32 RoachArrivalDist     = 0.15f; // snap to waypoint when this close
    static constexpr f32 RoachYawSpeed        = 6.0f;  // rad/s max turn rate
    static constexpr f32 RoachDetectionRange  = 10.0f; // player detection radius
    static constexpr f32 RoachAttackRange     = 1.5f;  // melee range
    static constexpr f32 RoachAttackRangeHyst = 2.0f;  // disengage above this
    static constexpr f32 RoachAttackRate      = 1.0f;  // seconds between hits
    static constexpr i32 RoachAttackDamage    = 10;
    static constexpr f32 RoachLostSightTime   = 3.0f;  // seconds before giving up chase
    static constexpr f32 RoachRePathInterval  = 1.5f;  // seconds between re-paths while chasing

    void Entity_Roach::PickPathTo( const Vec3 & dest ) {
        WaypointGraph & graph = map->GetNavGraph();
        if ( graph.GetNodeCount() < 2 ) { return; }

        i32 startNode = graph.FindNearestNode( position );
        i32 goalNode  = graph.FindNearestNode( dest );
        if ( startNode < 0 || goalNode < 0 || startNode == goalNode ) { return; }

        std::vector<i32> nodePath = graph.FindPath( startNode, goalNode );
        if ( (i32)nodePath.size() < 2 ) { return; }

        path.clear();
        for ( i32 idx : nodePath ) {
            path.push_back( graph.GetWaypoint( idx ).position );
        }
        pathIndex = 1;
        target = path[ pathIndex ];
        hasTarget = true;
    }

    bool Entity_Roach::HasLineOfSightTo( const Vec3 & tgt ) const {
        Vec3 toTarget = tgt - position;
        f32 dist = Length( toTarget );
        if ( dist < 0.01f ) { return true; }

        Vec3 dir = toTarget / dist;
        Vec3 rayStart = position + dir * 0.3f;

        MapRaycastResult hit;
        if ( map->Raycast( rayStart, dir, hit ) ) {
            if ( hit.entity == nullptr && hit.distance < dist - 0.3f ) {
                return false;
            }
        }
        return true;
    }

    bool Entity_Roach::CanSeePlayer() const {
        Vec3 playerPos = map->GetPlayerPosition();
        f32 dist = Distance( position, playerPos );
        if ( dist > RoachDetectionRange ) { return false; }
        return HasLineOfSightTo( playerPos );
    }

    void Entity_Roach::OnUpdate( f32 dt ) {
        RNG &rng = Engine::Get().GetRNG();
        Vec3 playerPos = map->GetPlayerPosition();
        f32 distToPlayer = Distance( position, playerPos );

        // ---- Per-frame: Chase / Attack ----
        switch ( state ) {
            case RoachState::Chase: {
                if ( CanSeePlayer() ) {
                    lostSightTimer = 0.0f;
                    if ( distToPlayer <= RoachAttackRange ) {
                        state = RoachState::Attack;
                        attackCooldown = 0.0f;
                        hasTarget = false;
                        path.clear();
                    } else {
                        rePathTimer -= dt;
                        if ( rePathTimer <= 0.0f || !hasTarget ) {
                            PickPathTo( playerPos );
                            rePathTimer = RoachRePathInterval;
                        }
                    }
                } else {
                    lostSightTimer += dt;
                    if ( lostSightTimer > RoachLostSightTime ) {
                        state = RoachState::Idle;
                        lostSightTimer = 0.0f;
                        hasTarget = false;
                        path.clear();
                    }
                }
            } break;

            case RoachState::Attack: {
                if ( distToPlayer > RoachAttackRangeHyst || !HasLineOfSightTo( playerPos ) ) {
                    state = RoachState::Chase;
                    rePathTimer = 0.0f;
                } else {
                    attackCooldown -= dt;
                    if ( attackCooldown <= 0.0f ) {
                        map->DamagePlayer( RoachAttackDamage );
                        attackCooldown = RoachAttackRate;
                    }
                }
            } break;

            default: break;
        }

        // ---- Think timer: Idle / Wonder ----
        thinkTimer -= dt;
        if ( thinkTimer <= 0.0f ) {
            thinkTimer += 1.0f;
            switch ( state ) {
                case RoachState::Idle: {
                    if ( CanSeePlayer() ) {
                        state = RoachState::Chase;
                        lostSightTimer = 0.0f;
                        rePathTimer = 0.0f;
                    } else if ( !hasTarget ) {
                        if ( rng.Float() > 0.55f ) {
                            state = RoachState::Wonder;
                        }
                    }
                } break;
                case RoachState::Wonder: {
                    if ( CanSeePlayer() ) {
                        state = RoachState::Chase;
                        lostSightTimer = 0.0f;
                        rePathTimer = 0.0f;
                        hasTarget = false;
                        path.clear();
                        break;
                    }
                    if ( !hasTarget ) {
                        WaypointGraph &graph = map->GetNavGraph();
                        const i32 nodeCount = graph.GetNodeCount();
                        if ( nodeCount < 2 ) { break; }

                        i32 startNode = graph.FindNearestNode( position );
                        if ( startNode < 0 ) { break; }

                        i32 goalNode = rng.Signed32( 0, nodeCount - 1 );
                        if ( goalNode == startNode ) {
                            goalNode = ( goalNode + 1 ) % nodeCount;
                        }

                        std::vector< i32 > nodePath = graph.FindPath( startNode, goalNode );
                        if ( ( i32 ) nodePath.size() < 2 ) { break; }

                        path.clear();
                        for ( i32 idx: nodePath ) {
                            path.push_back( graph.GetWaypoint( idx ).position );
                        }
                        pathIndex = 1;
                        target = path[ pathIndex ];
                        hasTarget = true;
                    }
                } break;
                default: break;
            }
        }

        // ---- Movement (Idle / Wonder / Chase) ----
        if ( hasTarget && state != RoachState::Attack ) {
            Vec3 toTarget = target - position;
            toTarget.y = 0.0f;
            f32 dist = Length( toTarget );

            if ( dist < RoachArrivalDist ) {
                pathIndex++;
                if ( pathIndex >= ( i32 ) path.size() ) {
                    hasTarget = false;
                    path.clear();
                    if ( state == RoachState::Wonder ) {
                        state = RoachState::Idle;
                    }
                } else {
                    target = path[ pathIndex ];
                }
            } else {
                Vec3 dir = toTarget / dist;
                position += dir * RoachMoveSpeed * dt;

                f32 targetYaw = atan2f( dir.x, dir.z );
                f32 yawDiff = targetYaw - facingYaw;
                while ( yawDiff > PI ) yawDiff -= TWO_PI;
                while ( yawDiff < -PI ) yawDiff += TWO_PI;
                facingYaw += Clamp( yawDiff, -RoachYawSpeed * dt, RoachYawSpeed * dt );
                orientation = Mat3( glm::angleAxis( facingYaw, Vec3( 0.0f, 1.0f, 0.0f ) ) );
            }
        }

        // ---- Orientation during attack (face player) ----
        if ( state == RoachState::Attack ) {
            Vec3 toPlayer = playerPos - position;
            toPlayer.y = 0.0f;
            f32 len = Length( toPlayer );
            if ( len > 0.01f ) {
                Vec3 dir = toPlayer / len;
                f32 targetYaw = atan2f( dir.x, dir.z );
                f32 yawDiff = targetYaw - facingYaw;
                while ( yawDiff > PI ) yawDiff -= TWO_PI;
                while ( yawDiff < -PI ) yawDiff += TWO_PI;
                facingYaw += Clamp( yawDiff, -RoachYawSpeed * dt, RoachYawSpeed * dt );
                orientation = Mat3( glm::angleAxis( facingYaw, Vec3( 0.0f, 1.0f, 0.0f ) ) );
            }
        }
    }

    void Entity_Roach::OnRender( Renderer &renderer ) {
        const Mat4 modelMatrix = GetModelMatrix();
        renderer.RenderStaticModel( model, modelMatrix );
    }

    void Entity_Roach::OnDespawn() {
    }

    AlignedBox Entity_Roach::GetBounds() const {
        ATTO_ASSERT( model != nullptr, "model = nullptr" );
        AlignedBox bounds = model->GetBounds();
        bounds.Translate( position );
        bounds.RotateAround( position, orientation );
        return bounds;
    }

    Box Entity_Roach::GetCollider() const {
        const AlignedBox bounds = model->GetBounds();
        Box box = Box::FromAlignedBox( bounds );
        box.center = position;
        box.orientation = orientation;
        return box;
    }

    bool Entity_Roach::RayTest( const Vec3 &start, const Vec3 &dir, f32 &dist ) const {
        const AlignedBox bounds = GetBounds();
        return Raycast::TestAlignedBox( start, dir, bounds, dist );
    }

    void Entity_Roach::Serialize( Serializer &serializer ) {
        Entity::Serialize( serializer );
    }

    void Entity_Roach::TakeDamage( i32 damage ) {
        health -= damage;
        if ( health <= 0 ) {
            map->DestroyEntity( this );
        }
    }

    void Entity_Roach::DebugDrawBounds( Renderer &renderer ) {
        const AlignedBox bounds = GetBounds();
        renderer.DebugAlignedBox( bounds );
    }

    void Entity_Roach::DebugDrawCollider( Renderer &renderer ) {
        const Box box = GetCollider();
        renderer.DebugBox( box );
    }

    void Entity_Roach::MoveTo( Vec3 target ) {
        hasTarget = true;
        this->target = target;
    }
}
