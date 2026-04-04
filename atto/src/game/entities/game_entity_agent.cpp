#include "game_entity_agent.h"

#include "game/game_map.h"

namespace atto {

    void Entity_Agent::PickPathTo( const Vec3 &dest ) {
        WaypointGraph &graph = map->GetNavGraph();
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
        pathIndex   = 1;
        agentTarget = path[ pathIndex ];
        hasTarget   = true;
    }

    bool Entity_Agent::HasLineOfSightTo( const Vec3 &tgt ) const {
        Vec3 toTarget = tgt - position;
        f32 dist = Length( toTarget );
        if ( dist < 0.01f ) { return true; }

        Vec3 dir      = toTarget / dist;
        Vec3 rayStart = position + dir * 0.3f;

        MapRaycastResult hit = {};
        if ( map->Raycast( rayStart, dir, hit ) ) {
            if ( hit.entity == nullptr && hit.distance < dist - 0.3f ) {
                return false;
            }
        }
        return true;
    }

    bool Entity_Agent::CanSeePlayer() const {
        Vec3 playerPos = map->GetPlayerPosition();
        f32 dist = Distance( position, playerPos );
        if ( dist > config.detectionRange ) { return false; }
        return HasLineOfSightTo( playerPos );
    }

    Vec3 Entity_Agent::Avoidance( Vec3 currentDirection, f32 dt ) {
        Vec3 steer( 0.0f );

        const i32 entityCount = map->GetEntityCount();
        for ( i32 i = 0; i < entityCount; i++ ) {
            Entity *other = map->GetEntity( i );
            if ( other == this || other->IsPendingDestroy() ) { continue; }

            Vec3 toOther = other->GetPosition() - position;
            toOther.y = 0.0f;
            f32 dist = Length( toOther );

            if ( dist < 0.01f ) {
                steer += Vec3( currentDirection.z, 0.0f, -currentDirection.x ) * config.avoidStrength;
                continue;
            }

            if ( dist > config.avoidRadius ) { continue; }

            f32  weight     = 1.0f - ( dist / config.avoidRadius );
            Vec3 away       = -toOther / dist;
            f32  forwardDot = Dot( currentDirection, toOther / dist );
            if ( forwardDot > 0.0f ) {
                weight *= ( 1.0f + forwardDot );
            }
            steer += away * weight * config.avoidStrength;
        }

        return steer;
    }

    void Entity_Agent::AgentOnUpdate( f32 dt ) {
        RNG &     rng       = Engine::Get().GetRNG();
        Vec3      playerPos = map->GetPlayerPosition();
        f32       distToPlayer = Distance( position, playerPos );

        // ---- Per-frame: Chase / Attack ----
        switch ( state ) {
            case AgentState::Chase: {
                if ( CanSeePlayer() ) {
                    lostSightTimer = 0.0f;
                    if ( distToPlayer <= config.attackRange ) {
                        state = AgentState::Attack;
                        hasTarget = false;
                        path.clear();
                    } else {
                        rePathTimer -= dt;
                        if ( rePathTimer <= 0.0f || !hasTarget ) {
                            PickPathTo( playerPos );
                            rePathTimer = config.rePathInterval;
                        }
                    }
                } else {
                    lostSightTimer += dt;
                    if ( lostSightTimer > config.lostSightTime ) {
                        state = AgentState::Idle;
                        lostSightTimer = 0.0f;
                        hasTarget = false;
                        path.clear();
                    }
                }
            } break;

            case AgentState::Attack: {
                if ( distToPlayer > config.attackRangeHyst || !HasLineOfSightTo( playerPos ) ) {
                    state = AgentState::Chase;
                    rePathTimer = 0.0f;
                } else {
                    attackCooldown -= dt;
                    if ( attackCooldown <= 0.0f ) {
                        OnAgentAttack();
                        attackCooldown = config.attackRate;
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
                case AgentState::Idle: {
                    if ( CanSeePlayer() ) {
                        state          = AgentState::Chase;
                        lostSightTimer = 0.0f;
                        rePathTimer    = 0.0f;
                    } else if ( !hasTarget ) {
                        if ( rng.Float() > 0.55f ) {
                            state = AgentState::Wonder;
                        }
                    }
                } break;
                case AgentState::Wonder: {
                    if ( CanSeePlayer() ) {
                        state          = AgentState::Chase;
                        lostSightTimer = 0.0f;
                        rePathTimer    = 0.0f;
                        hasTarget      = false;
                        path.clear();
                        break;
                    }
                    if ( !hasTarget ) {
                        WaypointGraph &graph     = map->GetNavGraph();
                        const i32      nodeCount = graph.GetNodeCount();
                        if ( nodeCount < 2 ) { break; }

                        i32 startNode = graph.FindNearestNode( position );
                        if ( startNode < 0 ) { break; }

                        i32 goalNode = rng.Signed32( 0, nodeCount - 1 );
                        if ( goalNode == startNode ) {
                            goalNode = ( goalNode + 1 ) % nodeCount;
                        }

                        std::vector<i32> nodePath = graph.FindPath( startNode, goalNode );
                        if ( (i32)nodePath.size() < 2 ) { break; }

                        path.clear();
                        for ( i32 idx : nodePath ) {
                            path.push_back( graph.GetWaypoint( idx ).position );
                        }
                        pathIndex   = 1;
                        agentTarget = path[ pathIndex ];
                        hasTarget   = true;
                    }
                } break;
                default: break;
            }
        }

        // ---- Movement (Idle / Wonder / Chase) ----
        if ( hasTarget && state != AgentState::Attack ) {
            Vec3 toTarget = agentTarget - position;
            toTarget.y    = 0.0f;
            const f32 dist = Length( toTarget );

            if ( dist < config.arrivalDist ) {
                pathIndex++;
                if ( pathIndex >= (i32)path.size() ) {
                    hasTarget = false;
                    path.clear();
                    if ( state == AgentState::Wonder ) {
                        state = AgentState::Idle;
                    }
                } else {
                    agentTarget = path[ pathIndex ];
                }
            } else {
                const Vec3 dir       = toTarget / dist;
                const Vec3 avoidance = Avoidance( dir, dt );
                Vec3       moveDir   = dir + avoidance;
                moveDir.y            = 0.0f;
                const f32 moveDirLen = Length( moveDir );
                if ( moveDirLen > 0.01f ) {
                    moveDir = moveDir / moveDirLen;
                }
                position += moveDir * config.moveSpeed * dt;

                walkSoundTimer -= dt;
                if ( walkSoundTimer <= 0.0f ) {
                    sndWalk.PlayAt( position, 20.0f );
                    walkSoundTimer = 1.0f;
                }

                const f32 targetYaw = atan2f( moveDir.x, moveDir.z );
                const f32 yawDiff   = NormalizeAngle( targetYaw - facingYaw );
                facingYaw  += Clamp( yawDiff, -config.yawSpeed * dt, config.yawSpeed * dt );
                orientation = Mat3( glm::angleAxis( facingYaw, Vec3( 0.0f, 1.0f, 0.0f ) ) );
            }
        }

        // ---- Damage flash ----
        if ( drawRed > 0.0f ) {
            drawRed = Max( drawRed - dt, 0.0f );
        }

        // ---- Orientation during attack (face player) ----
        if ( state == AgentState::Attack ) {
            Vec3 toPlayer = playerPos - position;
            toPlayer.y    = 0.0f;
            f32 len = Length( toPlayer );
            if ( len > 0.01f ) {
                const Vec3 dir      = toPlayer / len;
                const f32  targetYaw = atan2f( dir.x, dir.z );
                const f32  yawDiff   = NormalizeAngle( targetYaw - facingYaw );
                facingYaw  += Clamp( yawDiff, -config.yawSpeed * dt, config.yawSpeed * dt );
                orientation = Mat3( glm::angleAxis( facingYaw, Vec3( 0.0f, 1.0f, 0.0f ) ) );
            }
        }
    }

    void Entity_Agent::OnRender( Renderer &renderer ) {
        const Mat4 modelMatrix = GetModelMatrix();
        if ( drawRed > 0.0f ) {
            renderer.RenderStaticModel( model, modelMatrix, Vec3( 1.0f, 0.8f, 0.8f ) );
        } else {
            renderer.RenderStaticModel( model, modelMatrix );
        }

        if ( health < config.maxHealth ) {
            AlignedBox bounds = GetBounds();
            f32  barY   = bounds.max.y + 0.3f;
            Vec3 barPos = Vec3( position.x, barY, position.z );

            Vec3 cameraPos = map->GetPlayerPosition();
            Vec3 cameraUp  = map->GetPlayerCameraUp();
            if ( LengthSquared( cameraUp ) < 0.001f ) {
                cameraUp = Vec3( 0.0f, 1.0f, 0.0f );
            }

            f32  frac      = static_cast<f32>( health ) / static_cast<f32>( config.maxHealth );
            Vec4 fillColor = frac > 0.5f
                ? Vec4( 0.1f, 0.8f, 0.1f, 0.9f )
                : frac > 0.25f
                    ? Vec4( 1.0f, 0.7f, 0.0f, 0.9f )
                    : Vec4( 0.9f, 0.1f, 0.1f, 0.9f );

            renderer.RenderWorldBar( barPos, 0.8f, 0.08f, frac, fillColor, cameraPos, cameraUp );
        }
    }

    TakeDamageResult Entity_Agent::TakeDamage( i32 damage ) {
        drawRed = 0.2f;
        health -= damage;
        if ( health <= 0 ) {
            OnAgentDeath();
            map->DestroyEntity( this );
            sndDeath.PlayAt( position, 5.0f );
        }
        return TakeDamageResult::Success_HP;
    }

    void Entity_Agent::OnAgentDeath() {
        Entity *coin = map->CreateEntity( EntityType::Coin );
        if ( coin ) {
            coin->SetPosition( position + Vec3( 0.0f, 0.5f, 0.0f ) );
            coin->OnSpawn();
        }

        RNG &rng = Engine::Get().GetRNG();
        if ( rng.Float() < 0.1f ) {
            Entity *hp = map->CreateEntity( EntityType::HealthPickup );
            if ( hp ) {
                hp->SetPosition( position + Vec3( 0.0f, 0.8f, 0.0f ) );
                hp->OnSpawn();
            }
        }
    }

    AlignedBox Entity_Agent::GetBounds() const {
        ATTO_ASSERT( model != nullptr, "model = nullptr" );
        AlignedBox bounds = model->GetBounds();
        bounds.Translate( position );
        bounds.RotateAround( position, orientation );
        return bounds;
    }

    Box Entity_Agent::GetCollider() const {
        const AlignedBox bounds = model->GetBounds();
        Box box       = Box::FromAlignedBox( bounds );
        box.center    = position;
        box.orientation = orientation;
        return box;
    }

    bool Entity_Agent::RayTest( const Vec3 &start, const Vec3 &dir, f32 &dist ) const {
        const AlignedBox bounds = GetBounds();
        return Raycast::TestAlignedBox( start, dir, bounds, dist );
    }

    void Entity_Agent::Serialize( Serializer &serializer ) {
        Entity::Serialize( serializer );
    }

    void Entity_Agent::DebugDrawBounds( Renderer &renderer ) {
        const AlignedBox bounds = GetBounds();
        renderer.DebugAlignedBox( bounds );
    }

    void Entity_Agent::DebugDrawCollider( Renderer &renderer ) {
        const Box box = GetCollider();
        renderer.DebugBox( box );
        DebugDrawPath( renderer );
    }

    void Entity_Agent::DebugDrawPath( Renderer &renderer ) {
        if ( path.empty() ) { return; }

        static const Vec3 colorPast( 0.4f, 0.4f, 0.4f );
        static const Vec3 colorActive( 1.0f, 1.0f, 0.0f );
        static const Vec3 colorFuture( 0.0f, 1.0f, 0.0f );
        static const Vec3 colorWaypoint( 1.0f, 0.5f, 0.0f );
        static const f32  waypointRadius = 0.1f;
        static const f32  liftY          = 0.05f;

        Vec3 pos = position;
        pos.y += liftY;
        Vec3 tgt = agentTarget;
        tgt.y += liftY;
        renderer.DebugLine( pos, tgt, colorActive );

        for ( i32 i = 0; i < (i32)path.size() - 1; i++ ) {
            Vec3       a     = path[ i ];
            Vec3       b     = path[ i + 1 ];
            a.y += liftY;
            b.y += liftY;
            const Vec3 &color = ( i + 1 < pathIndex ) ? colorPast : colorFuture;
            renderer.DebugLine( a, b, color );
        }

        for ( i32 i = 0; i < (i32)path.size(); i++ ) {
            Vec3 p = path[ i ];
            p.y += liftY;
            renderer.DebugSphere( Sphere{ p, waypointRadius }, ( i == pathIndex ) ? colorActive : colorWaypoint );
        }
    }

    void Entity_Agent::MoveTo( Vec3 target ) {
        hasTarget   = true;
        agentTarget = target;
    }
}
