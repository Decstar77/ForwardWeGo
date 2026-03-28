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

        sndAttack.Initialize();
        sndAttack.LoadSounds( {
            "assets/sounds/roach/attack.wav"
        } );

        sndWalk.Initialize();
        sndWalk.LoadSounds( {
            "assets/sounds/roach/walk.wav"
        });
    }

    static constexpr f32 RoachMoveSpeed       = 2.8f;  // m/s
    static constexpr f32 RoachArrivalDist     = 0.65; // snap to waypoint when this close
    static constexpr f32 RoachYawSpeed        = 6.0f;  // rad/s max turn rate
    static constexpr f32 RoachDetectionRange  = 50.0f; // player detection radius
    static constexpr f32 RoachAttackRange     = 2.0f;  // melee range
    static constexpr f32 RoachAttackRangeHyst = 2.0f;  // disengage above this
    static constexpr f32 RoachAttackRate      = 1.0f;  // seconds between hits
    static constexpr i32 RoachAttackDamage    = 10;
    static constexpr f32 RoachLostSightTime   = 3.0f;  // seconds before giving up chase
    static constexpr f32 RoachRePathInterval  = 1.5f;  // seconds between re-paths while chasing
    static constexpr f32 RoachAvoidRadius     = 0.75f;  // how far ahead/around to check for obstacles
    static constexpr f32 RoachAvoidStrength   = 3.0f;  // steering force multiplier

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

        MapRaycastResult hit = {};
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

    Vec3 Entity_Roach::Avoidance( Vec3 currentDirection, f32 dt ) {
        Vec3 steer( 0.0f );

        // Avoid other entities
        const i32 entityCount = map->GetEntityCount();
        for ( i32 i = 0; i < entityCount; i++ ) {
            Entity * other = map->GetEntity( i );
            if ( other == this || other->IsPendingDestroy() ) { continue; }

            Vec3 toOther = other->GetPosition() - position;
            toOther.y = 0.0f;
            f32 dist = Length( toOther );

            if ( dist < 0.01f ) {
                // Overlapping exactly, push in perpendicular direction
                steer += Vec3( currentDirection.z, 0.0f, -currentDirection.x ) * RoachAvoidStrength;
                continue;
            }

            if ( dist > RoachAvoidRadius ) { continue; }

            // Stronger repulsion when closer
            f32 weight = 1.0f - ( dist / RoachAvoidRadius );
            Vec3 away = -toOther / dist;

            // Scale up avoidance for obstacles that are ahead of us
            f32 forwardDot = Dot( currentDirection, toOther / dist );
            if ( forwardDot > 0.0f ) {
                weight *= ( 1.0f + forwardDot );
            }

            steer += away * weight * RoachAvoidStrength;
        }

        // Avoid brush colliders
        /*
        const i32 brushCount = map->GetBrushCount();
        for ( i32 i = 0; i < brushCount; i++ ) {
            const Brush & brush = map->GetBrush( i );
            AlignedBox box = AlignedBox::FromCenterSize( brush.center, brush.halfExtents * 2.0f );

            Vec3 closest = box.ClosestPoint( position );
            Vec3 toWall = closest - position;
            toWall.y = 0.0f;
            f32 dist = Length( toWall );

            if ( dist < 0.01f ) {
                // Inside the brush, push perpendicular to movement
                //steer += Vec3( currentDirection.z, 0.0f, -currentDirection.x ) * RoachAvoidStrength;
                continue;
            }

            if ( dist > RoachAvoidRadius ) { continue; }

            f32 weight = 1.0f - ( dist / RoachAvoidRadius );

            f32 forwardDot = Dot( currentDirection, toWall / dist );
            if ( forwardDot > 0.0f ) {
                weight *= ( 1.0f + forwardDot );
            }

            Vec3 away = -toWall / dist;
            steer += away * weight * RoachAvoidStrength;
        }
        */
        return steer;
    }

    void Entity_Roach::OnUpdate( f32 dt ) {
        RNG &rng = Engine::Get().GetRNG();
        const Vec3 playerPos = map->GetPlayerPosition();
        const f32 distToPlayer = Distance( position, playerPos );

        // ---- Per-frame: Chase / Attack ----
        switch ( state ) {
            case RoachState::Chase: {
                if ( CanSeePlayer() ) {
                    lostSightTimer = 0.0f;
                    if ( distToPlayer <= RoachAttackRange ) {
                        state = RoachState::Attack;
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
                        sndAttack.PlayAt( position, 10.0f );
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
            const f32 dist = Length( toTarget );

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
                const Vec3 dir = toTarget / dist;
                const Vec3 avoidance = Avoidance( dir, dt );
                Vec3 moveDir = dir + avoidance;
                moveDir.y = 0.0f;
                const f32 moveDirLen = Length( moveDir );
                if ( moveDirLen > 0.01f ) {
                    moveDir = moveDir / moveDirLen;
                }
                position += moveDir * RoachMoveSpeed * dt;

                walkSoundTimer -= dt;
                if ( walkSoundTimer <= 0.0f ) {
                    sndWalk.PlayAt( position, 20.0f );
                    walkSoundTimer = 1.0f;
                }

                const f32 targetYaw = atan2f( moveDir.x, moveDir.z );
                const f32 yawDiff = NormalizeAngle( targetYaw - facingYaw );
                facingYaw += Clamp( yawDiff, -RoachYawSpeed * dt, RoachYawSpeed * dt );
                orientation = Mat3( glm::angleAxis( facingYaw, Vec3( 0.0f, 1.0f, 0.0f ) ) );
            }
        }

        // ---- Damage flash ----
        if ( drawRed > 0.0f ) {
            drawRed = Max( drawRed - dt, 0.0f );
        }

        // ---- Orientation during attack (face player) ----
        if ( state == RoachState::Attack ) {
            Vec3 toPlayer = playerPos - position;
            toPlayer.y = 0.0f;
            f32 len = Length( toPlayer );
            if ( len > 0.01f ) {
                const Vec3 dir = toPlayer / len;
                const f32 targetYaw = atan2f( dir.x, dir.z );
                const f32 yawDiff = NormalizeAngle( targetYaw - facingYaw );
                facingYaw += Clamp( yawDiff, -RoachYawSpeed * dt, RoachYawSpeed * dt );
                orientation = Mat3( glm::angleAxis( facingYaw, Vec3( 0.0f, 1.0f, 0.0f ) ) );
            }
        }
    }

    void Entity_Roach::OnRender( Renderer &renderer ) {
        const Mat4 modelMatrix = GetModelMatrix();
        if ( drawRed > 0.0f ) {
            renderer.RenderStaticModel( model, modelMatrix, Vec3( 1.0f, 0.8f, 0.8f ) );
        }
        else {
            renderer.RenderStaticModel( model, modelMatrix );
        }

        // Health bar above entity
        if ( health < MAX_HEALTH ) {
            AlignedBox bounds = GetBounds();
            f32 barY = bounds.max.y + 0.3f;
            Vec3 barPos = Vec3( position.x, barY, position.z );

            Vec3 cameraPos = map->GetPlayerPosition();
            Vec3 cameraUp = map->GetPlayerCameraUp();
            if ( LengthSquared( cameraUp ) < 0.001f ) {
                cameraUp = Vec3( 0.0f, 1.0f, 0.0f );
            }

            f32 frac = static_cast<f32>( health ) / static_cast<f32>( MAX_HEALTH );
            Vec4 fillColor = frac > 0.5f
                ? Vec4( 0.1f, 0.8f, 0.1f, 0.9f )
                : frac > 0.25f
                    ? Vec4( 1.0f, 0.7f, 0.0f, 0.9f )
                    : Vec4( 0.9f, 0.1f, 0.1f, 0.9f );

            renderer.RenderWorldBar( barPos, 0.8f, 0.08f, frac, fillColor, cameraPos, cameraUp );
        }
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

    TakeDamageResult Entity_Roach::TakeDamage( i32 damage ) {
        drawRed = 0.2f;
        health -= damage;
        if ( health <= 0 ) {
            // Spawn a coin at the roach's position
            Entity * coin = map->CreateEntity( EntityType::Coin );
            if ( coin ) {
                coin->SetPosition( position + Vec3( 0.0f, 0.5f, 0.0f ) );
                coin->OnSpawn();
            }

            // 10% chance to spawn a health pickup
            RNG & rng = Engine::Get().GetRNG();
            if ( rng.Float() < 0.1f ) {
                Entity * hp = map->CreateEntity( EntityType::HealthPickup );
                if ( hp ) {
                    hp->SetPosition( position + Vec3( 0.0f, 0.8f, 0.0f ) );
                    hp->OnSpawn();
                }
            }

            map->DestroyEntity( this );
        }

        return TakeDamageResult::Success_HP;
    }

    void Entity_Roach::DebugDrawBounds( Renderer &renderer ) {
        const AlignedBox bounds = GetBounds();
        renderer.DebugAlignedBox( bounds );
    }

    void Entity_Roach::DebugDrawCollider( Renderer &renderer ) {
        const Box box = GetCollider();
        renderer.DebugBox( box );
        DebugDrawPath(renderer);
    }

    void Entity_Roach::DebugDrawPath( Renderer & renderer ) {
        if ( path.empty() ) { return; }

        static const Vec3 colorPast( 0.4f, 0.4f, 0.4f );
        static const Vec3 colorActive( 1.0f, 1.0f, 0.0f );
        static const Vec3 colorFuture( 0.0f, 1.0f, 0.0f );
        static const Vec3 colorWaypoint( 1.0f, 0.5f, 0.0f );
        static const f32 waypointRadius = 0.1f;
        static const f32 liftY = 0.05f;

        // Draw line from roach to current target
        Vec3 pos = position;
        pos.y += liftY;
        Vec3 tgt = target;
        tgt.y += liftY;
        renderer.DebugLine( pos, tgt, colorActive );

        // Draw path segments
        for ( i32 i = 0; i < ( i32 ) path.size() - 1; i++ ) {
            Vec3 a = path[i];
            Vec3 b = path[i + 1];
            a.y += liftY;
            b.y += liftY;

            const Vec3 & color = ( i + 1 < pathIndex ) ? colorPast : colorFuture;
            renderer.DebugLine( a, b, color );
        }

        // Draw waypoint markers
        for ( i32 i = 0; i < ( i32 ) path.size(); i++ ) {
            Vec3 p = path[i];
            p.y += liftY;
            renderer.DebugSphere( Sphere{ p, waypointRadius }, ( i == pathIndex ) ? colorActive : colorWaypoint );
        }
    }

    void Entity_Roach::MoveTo( Vec3 target ) {
        hasTarget = true;
        this->target = target;
    }
}
