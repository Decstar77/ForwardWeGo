#include "game_entity_drone_quad.h"
#include "../game_map.h"

namespace atto {
    ATTO_REGISTER_CLASS(  Entity, Entity_DroneQuad, EntityType::Drone_QUAD )

    // -------------------------------------------------------
    //  Tuning constants
    // -------------------------------------------------------
    // Movement
    static constexpr f32 DroneMoveMaxSpeed    = 4.0f;   // m/s top speed
    static constexpr f32 DroneMoveAccel       = 8.0f;   // steering lerp rate (higher = snappier)
    static constexpr f32 DroneSlowdownDist    = 3.0f;   // distance at which braking begins (final target only)
    static constexpr f32 DroneArrivalDist     = 0.12f;  // snap-to-target threshold (final target)
    static constexpr f32 DronePassThroughDist = 0.5f;   // advance to next point when this close to an intermediate
    // Orientation
    static constexpr f32 DroneYawSpeed        = 3.5f;   // max yaw turn rate (rad/s)
    static constexpr f32 DroneYawLerpRate     = 6.0f;
    static constexpr f32 DronePitchLerpRate   = 4.5f;
    static constexpr f32 DroneRollLerpRate    = 3.5f;
    static constexpr f32 DronePitchFactor     = 0.04f;  // rad per (m/s) of forward speed
    static constexpr f32 DroneBankFactor      = 0.015f; // rad per (rad/s * m/s)
    // Hover
    static constexpr f32 DroneHoverAmpY       = 0.035f; // metres of vertical bob
    static constexpr f32 DroneHoverFreqY      = 1.7f;   // bob cycles per second
    // Path smoothing
    static constexpr i32 DronePathSubdivisions = 4;     // Catmull-Rom samples per segment
    // Vision
    static constexpr f32 DroneVisionRange     = 25.0f;  // max sight distance
    static constexpr f32 DroneVisionHalfAngle = 1.047f; // ~60 deg half-angle (120 deg cone)
    // Combat
    static constexpr f32 DroneAttackRange     = 5.0f;   // begin firing when this close
    static constexpr f32 DroneAttackRangeHyst = 6.5f;   // disengage attack above this
    static constexpr f32 DroneAttackFireRate  = 0.5f;   // seconds between shots
    static constexpr i32 DroneAttackDamage    = 5;      // damage per shot
    static constexpr f32 DroneLostSightTime   = 3.0f;   // seconds before giving up chase
    static constexpr f32 DroneRePathInterval  = 1.0f;   // seconds between re-pathing while chasing
    // -------------------------------------------------------

    Entity_DroneQuad::Entity_DroneQuad() {
        type = EntityType::Drone_QUAD;
    }

    void Entity_DroneQuad::OnSpawn() {
        Renderer & renderer = Engine::Get().GetRenderer();
        model = renderer.GetOrLoadStaticModel( "assets/models/sm/SM_Prop_Drone_Quad_01.obj" );
        basePosition = position;
    }

    void Entity_DroneQuad::MoveTo( const Vec3 & target ) {
        moveTarget = target;
        hasTarget = true;
    }

    void Entity_DroneQuad::Serialize( Serializer & serializer ) {
        Entity::Serialize( serializer );
    }

    void Entity_DroneQuad::BuildSmoothedPath( const std::vector<i32> & nodePath ) {
        WaypointGraph & navGraph = map->GetNavGraph();

        std::vector<Vec3> rawPoints;
        rawPoints.reserve( nodePath.size() );
        for ( i32 idx : nodePath ) {
            rawPoints.push_back( navGraph.GetWaypoint( idx ).position );
        }

        smoothedPath.clear();
        const i32 count = (i32)rawPoints.size();
        for ( i32 i = 0; i < count - 1; i++ ) {
            Vec3 p0 = ( i > 0 )         ? rawPoints[ i - 1 ] : 2.0f * rawPoints[ 0 ] - rawPoints[ 1 ];
            Vec3 p1 = rawPoints[ i ];
            Vec3 p2 = rawPoints[ i + 1 ];
            Vec3 p3 = ( i + 2 < count ) ? rawPoints[ i + 2 ] : 2.0f * rawPoints[ count - 1 ] - rawPoints[ count - 2 ];

            for ( i32 s = 0; s < DronePathSubdivisions; s++ ) {
                f32 t  = (f32)s / (f32)DronePathSubdivisions;
                f32 t2 = t * t;
                f32 t3 = t2 * t;
                Vec3 point = 0.5f * (
                    ( 2.0f * p1 ) +
                    ( -p0 + p2 ) * t +
                    ( 2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3 ) * t2 +
                    ( -p0 + 3.0f * p1 - 3.0f * p2 + p3 ) * t3
                );
                smoothedPath.push_back( point );
            }
        }
        smoothedPath.push_back( rawPoints.back() );

        smoothedPathIndex = 1;
        moveTarget = smoothedPath[ smoothedPathIndex ];
        hasTarget = true;
    }

    void Entity_DroneQuad::PickNewWanderTarget() {
        WaypointGraph & navGraph = map->GetNavGraph();
        const i32 nodeCount = navGraph.GetNodeCount();
        if ( nodeCount < 2 ) {
            return;
        }

        i32 startNode = navGraph.FindNearestNode( basePosition );
        if ( startNode < 0 ) {
            return;
        }

        RNG & rng = Engine::Get().GetRNG();
        i32 goalNode = rng.Signed32( 0, nodeCount - 1 );
        if ( goalNode == startNode ) {
            goalNode = ( goalNode + 1 ) % nodeCount;
        }

        std::vector<i32> path = navGraph.FindPath( startNode, goalNode );
        if ( (i32)path.size() < 2 ) {
            return;
        }

        BuildSmoothedPath( path );
    }

    void Entity_DroneQuad::PathToward( const Vec3 & target ) {
        WaypointGraph & navGraph = map->GetNavGraph();
        if ( navGraph.GetNodeCount() < 2 ) {
            return;
        }

        i32 startNode = navGraph.FindNearestNode( basePosition );
        i32 goalNode  = navGraph.FindNearestNode( target );
        if ( startNode < 0 || goalNode < 0 || startNode == goalNode ) {
            MoveTo( target );
            smoothedPath.clear();
            smoothedPathIndex = 0;
            return;
        }

        std::vector<i32> path = navGraph.FindPath( startNode, goalNode );
        if ( (i32)path.size() < 2 ) {
            MoveTo( target );
            smoothedPath.clear();
            smoothedPathIndex = 0;
            return;
        }

        BuildSmoothedPath( path );
    }

    bool Entity_DroneQuad::HasLineOfSightTo( const Vec3 & target ) const {
        Vec3 toTarget = target - basePosition;
        f32 dist = Length( toTarget );
        if ( dist < 0.01f ) {
            return true;
        }

        Vec3 dir = toTarget / dist;
        Vec3 rayStart = basePosition + dir * 0.5f;

        MapRaycastResult hit;
        if ( map->Raycast( rayStart, dir, hit ) ) {
            if ( hit.entity == nullptr && hit.distance < dist - 0.5f ) {
                return false;
            }
        }

        return true;
    }

    bool Entity_DroneQuad::CanSeePlayer() const {
        Vec3 playerPos = map->GetPlayerPosition();
        Vec3 toPlayer = playerPos - basePosition;
        f32 dist = Length( toPlayer );

        if ( dist > DroneVisionRange || dist < 0.01f ) {
            return false;
        }

        Vec3 facing = Vec3( sinf( smoothYaw ), 0.0f, cosf( smoothYaw ) );
        Vec3 dirXZ = Normalize( Vec3( toPlayer.x, 0.0f, toPlayer.z ) );
        f32 dot = Dot( dirXZ, facing );
        if ( dot < cosf( DroneVisionHalfAngle ) ) {
            return false;
        }

        return HasLineOfSightTo( playerPos );
    }

    void Entity_DroneQuad::OnUpdate( f32 dt ) {
        hoverTime += dt;

        Vec3 playerPos = map->GetPlayerPosition();
        f32 distToPlayer = Distance( basePosition, playerPos );

        // =============================================
        //  AI State Machine
        // =============================================
        switch ( aiState ) {
        case DroneAIState::Wander: {
            if ( CanSeePlayer() ) {
                aiState = DroneAIState::Chase;
                lostSightTimer = 0.0f;
                rePathTimer = 0.0f;
                PathToward( playerPos );
            }
            else if ( !hasTarget ) {
                PickNewWanderTarget();
            }
        } break;

        case DroneAIState::Chase: {
            bool canSee = CanSeePlayer();
            if ( canSee ) {
                lostSightTimer = 0.0f;
                if ( distToPlayer <= DroneAttackRange && HasLineOfSightTo( playerPos ) ) {
                    aiState = DroneAIState::Attack;
                    attackCooldown = 0.0f;
                    hasTarget = false;
                    smoothedPath.clear();
                }
                else {
                    rePathTimer -= dt;
                    if ( rePathTimer <= 0.0f || !hasTarget ) {
                        PathToward( playerPos );
                        rePathTimer = DroneRePathInterval;
                    }
                }
            }
            else {
                lostSightTimer += dt;
                if ( lostSightTimer > DroneLostSightTime || !hasTarget ) {
                    aiState = DroneAIState::Wander;
                    lostSightTimer = 0.0f;
                }
            }
        } break;

        case DroneAIState::Attack: {
            if ( distToPlayer > DroneAttackRangeHyst || !HasLineOfSightTo( playerPos ) ) {
                aiState = DroneAIState::Chase;
                rePathTimer = 0.0f;
                PathToward( playerPos );
            }
            else {
                attackCooldown -= dt;
                if ( attackCooldown <= 0.0f ) {
                    map->DamagePlayer( DroneAttackDamage );
                    attackCooldown = DroneAttackFireRate;
                }
            }
        } break;
        }

        // =============================================
        //  Movement
        // =============================================
        if ( hasTarget ) {
            Vec3 toTarget = moveTarget - basePosition;
            f32  dist = Length( toTarget );
            bool isFinalTarget = smoothedPathIndex >= (i32)smoothedPath.size();

            if ( isFinalTarget && dist < DroneArrivalDist ) {
                basePosition = moveTarget;
                velocity = Vec3( 0.0f );
                hasTarget = false;
            }
            else if ( !isFinalTarget && dist < DronePassThroughDist ) {
                moveTarget = smoothedPath[ smoothedPathIndex ];
                smoothedPathIndex++;
            }
            else {
                Vec3 dir = toTarget / dist;
                f32  desiredSpeed;
                if ( isFinalTarget ) {
                    desiredSpeed = DroneMoveMaxSpeed * SmoothStep( 0.0f, DroneSlowdownDist, dist );
                }
                else {
                    desiredSpeed = DroneMoveMaxSpeed;
                }
                Vec3 desiredVel = dir * desiredSpeed;
                velocity = Lerp( velocity, desiredVel, Clamp( DroneMoveAccel * dt, 0.0f, 1.0f ) );
            }
        }
        else {
            velocity = Lerp( velocity, Vec3( 0.0f ), Clamp( DroneMoveAccel * dt, 0.0f, 1.0f ) );
        }

        basePosition += velocity * dt;

        // =============================================
        //  Orientation
        // =============================================
        f32 yawRate = 0.0f;

        if ( aiState == DroneAIState::Attack ) {
            Vec3 toPlayer = playerPos - basePosition;
            f32 targetYaw = atan2f( toPlayer.x, toPlayer.z );

            f32 yawDiff = targetYaw - smoothYaw;
            while ( yawDiff > PI ) yawDiff -= TWO_PI;
            while ( yawDiff < -PI ) yawDiff += TWO_PI;

            f32 yawStep = Clamp( yawDiff, -DroneYawSpeed * dt, DroneYawSpeed * dt );
            yawRate = ( dt > 0.0f ) ? ( yawStep / dt ) : 0.0f;
            smoothYaw += yawStep;
        }
        else {
            Vec2 horizVel = Vec2( velocity.x, velocity.z );
            f32  horizSpeed = Length( horizVel );

            if ( horizSpeed > 0.1f ) {
                f32 targetYaw = atan2f( velocity.x, velocity.z );

                f32 yawDiff = targetYaw - smoothYaw;
                while ( yawDiff > PI ) yawDiff -= TWO_PI;
                while ( yawDiff < -PI ) yawDiff += TWO_PI;

                f32 yawStep = Clamp( yawDiff, -DroneYawSpeed * dt, DroneYawSpeed * dt );
                yawRate = ( dt > 0.0f ) ? ( yawStep / dt ) : 0.0f;
                smoothYaw += yawStep;
            }
        }

        Vec2 horizVel = Vec2( velocity.x, velocity.z );
        f32  horizSpeed = Length( horizVel );

        // ---- Pitch ----
        Vec3 facing = Vec3( sinf( smoothYaw ), 0.0f, cosf( smoothYaw ) );
        f32  forwardSpd = Dot( velocity, facing );
        f32  targetPitch = -forwardSpd * DronePitchFactor;
        targetPitch += sinf( hoverTime * 0.89f + 1.3f ) * 0.006f;
        smoothPitch = Lerp( smoothPitch, targetPitch, Clamp( DronePitchLerpRate * dt, 0.0f, 1.0f ) );

        // ---- Roll ----
        f32 targetRoll = yawRate * horizSpeed * DroneBankFactor;
        targetRoll += sinf( hoverTime * 0.73f ) * 0.008f;
        smoothRoll = Lerp( smoothRoll, targetRoll, Clamp( DroneRollLerpRate * dt, 0.0f, 1.0f ) );

        // ---- Build orientation ----
        Quat yawQ   = glm::angleAxis( smoothYaw,   Vec3( 0.0f, 1.0f, 0.0f ) );
        Quat pitchQ = glm::angleAxis( smoothPitch, Vec3( 1.0f, 0.0f, 0.0f ) );
        Quat rollQ  = glm::angleAxis( smoothRoll,  Vec3( 0.0f, 0.0f, 1.0f ) );
        orientation = Mat3( yawQ * pitchQ * rollQ );

        // ---- Hover bob ----
        f32 hoverOffsetY = sinf( hoverTime * DroneHoverFreqY ) * DroneHoverAmpY;
        position = basePosition + Vec3( 0.0f, hoverOffsetY, 0.0f );
    }

    void Entity_DroneQuad::OnRender( Renderer & renderer ) {
        Mat4 modelMatrix = GetModelMatrix();
        renderer.RenderStaticModel( model, modelMatrix );
    }

    void Entity_DroneQuad::OnDespawn() {
    }

    AlignedBox Entity_DroneQuad::GetBounds() const {
        AlignedBox bounds = model->GetBounds();
        bounds.Translate( position );
        bounds.RotateAround( position, orientation );
        return bounds;
    }

    bool Entity_DroneQuad::RayTest( const Vec3 & start, const Vec3 & dir, f32 & dist ) const {
        AlignedBox bounds = GetBounds();
        return Raycast::TestAlignedBox( start, dir, bounds, dist );
    }

    void Entity_DroneQuad::DebugDrawBounds( Renderer & renderer ) {
        AlignedBox bounds = GetBounds();
        renderer.DebugAlignedBox( bounds );
    }

    TakeDamageResult Entity_DroneQuad::TakeDamage( i32 damage ) {
        health -= damage;
        if ( health <= 0 ) {
            map->DestroyEntity( this );
        }

        return TakeDamageResult::Success_HP;
    }
}
