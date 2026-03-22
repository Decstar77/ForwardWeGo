#include "game_entities.h"

#include "game_map.h"

namespace atto {
    Mat4 Entity::GetModelMatrix() const {
        Mat4 modelMatrix = Mat4( 1.0f );
        modelMatrix = glm::translate( modelMatrix, position ) * Mat4( orientation );
        return modelMatrix;
    }

    void Entity::Serialize( Serializer & serializer ) {
        // @SPEED: We should use a SmallString for this but the serializer doesn't support it yet
        if ( serializer.IsSaving() ) {
            std::string typeStr = EntityTypeToString( type );
            serializer( "Type", typeStr );
        }
        else {
            std::string typeStr;
            serializer( "Type", typeStr );
            type = StringToEntityType( typeStr.c_str() );
        }

        serializer( "SpawnId", spawnId );
        serializer( "Position", position );
        serializer( "Orientation", orientation );
    }

    Entity_Barrel::Entity_Barrel() {
        type = EntityType::Barrel;
    }

    void Entity_Barrel::OnSpawn() {
        Renderer & renderer = Engine::Get().GetRenderer();
        model = renderer.GetOrLoadStaticModel( "assets/player/props/barrel.fbx", 3.5f );
        orientation = Mat3( glm::rotate( glm::mat4( 1 ), glm::radians( -90.0f ), Vec3( 1.0f, 0.0f, 0.0f ) ) );
    }

    void Entity_Barrel::OnUpdate( f32 dt ) {
    }

    void Entity_Barrel::OnRender( Renderer & renderer ) {
        ATTO_ASSERT( model != nullptr, "Model is null" );

        Mat4 modelMatrix = GetModelMatrix();
        renderer.RenderStaticModel( model, modelMatrix );
    }

    void Entity_Barrel::OnDespawn() {

    }

    AlignedBox Entity_Barrel::GetBounds() const {
        ATTO_ASSERT( model != nullptr, "Model is null" );
        AlignedBox bounds = model->GetBounds();
        bounds.Translate( position );
        bounds.RotateAround( position, orientation );
        return bounds;
    }

    bool Entity_Barrel::RayTest( const Vec3 & start, const Vec3 & dir, f32 & dist ) const {
        AlignedBox bounds = GetBounds();
        return Raycast::TestAlignedBox( start, dir, bounds, dist );
    }

    void Entity_Barrel::DebugDrawBounds( Renderer & renderer ) {
        AlignedBox bounds = GetBounds();
        renderer.DebugAlignedBox( bounds );
    }

    void Entity_Barrel::TakeDamage( i32 damage ) {
        health -= damage;
        if ( health <= 0 ) {
            map->DestroyEntity( this );
        }
    };

    Entity_Prop::Entity_Prop() {
        type = EntityType::Prop;
    }

    void Entity_Prop::OnSpawn() {

    }

    void Entity_Prop::OnUpdate( f32 dt ) {

    }

    void Entity_Prop::OnRender( Renderer & renderer ) {
        if ( model != nullptr ) {
            Mat4 modelMatrix = GetModelMatrix();
            renderer.RenderStaticModel( model, modelMatrix );
        }
    }

    void Entity_Prop::OnDespawn() {

    }

    AlignedBox Entity_Prop::GetBounds() const {
        if ( model == nullptr ) {
            return {};
        }

        AlignedBox bounds = model->GetBounds();
        bounds.Translate( position );
        bounds.RotateAround( position, orientation );
        return bounds;
    }

    bool Entity_Prop::RayTest( const Vec3 & start, const Vec3 & dir, f32 & dist ) const {
        AlignedBox bounds = GetBounds();
        return Raycast::TestAlignedBox( start, dir, bounds, dist );
    }

    void Entity_Prop::DebugDrawBounds( Renderer & renderer ) {
        AlignedBox bounds = GetBounds();
        renderer.DebugAlignedBox( bounds );
    }

    void Entity_Prop::Serialize( Serializer & serializer ) {
        Entity::Serialize( serializer );
        serializer( "model", model );
    }

    Entity_ExitDoor::Entity_ExitDoor() {
        type = EntityType::ExitDoor;
    }

    void Entity_ExitDoor::OnSpawn() {
        Renderer & renderer = Engine::Get().GetRenderer();
        modelClosed = renderer.GetOrLoadStaticModel( "assets/models/sm-declan/SM_Bld_Section_Door_06_Closed.obj" );
    }

    void Entity_ExitDoor::OnUpdate( f32 dt ) {

    }

    void Entity_ExitDoor::OnRender( Renderer & renderer ) {
        Mat4 modelMatrix = GetModelMatrix();
        renderer.RenderStaticModel( modelClosed, modelMatrix );
    }

    void Entity_ExitDoor::OnDespawn() {

    }

    AlignedBox Entity_ExitDoor::GetBounds() const {
        AlignedBox bounds = modelClosed->GetBounds();
        bounds.Translate( position );
        bounds.RotateAround( position, orientation );
        return bounds;
    }

    bool Entity_ExitDoor::RayTest( const Vec3 & start, const Vec3 & dir, f32 & dist ) const {
        AlignedBox bounds = GetBounds();
        return Raycast::TestAlignedBox( start, dir, bounds, dist );
    }

    void Entity_ExitDoor::DebugDrawBounds( Renderer & renderer ) {
        AlignedBox bounds = GetBounds();
        renderer.DebugAlignedBox( bounds );
    }



    // -------------------------------------------------------
    //  Tuning constants
    // -------------------------------------------------------
    static constexpr f32 DroneMoveMaxSpeed = 4.0f;   // m/s top speed
    static constexpr f32 DroneMoveAccel = 8.0f;   // steering lerp rate (higher = snappier)
    static constexpr f32 DroneSlowdownDist = 3.0f;   // distance at which braking begins (final target only)
    static constexpr f32 DroneArrivalDist = 0.12f;  // snap-to-target threshold (final target)
    static constexpr f32 DronePassThroughDist = 0.5f;  // advance to next point when this close to an intermediate
    static constexpr f32 DroneYawSpeed = 3.5f;   // max yaw turn rate (rad/s)
    static constexpr f32 DroneYawLerpRate = 6.0f;
    static constexpr f32 DronePitchLerpRate = 4.5f;
    static constexpr f32 DroneRollLerpRate = 3.5f;
    static constexpr f32 DronePitchFactor = 0.04f;  // rad per (m/s) of forward speed
    static constexpr f32 DroneBankFactor = 0.015f; // rad per (rad/s * m/s)
    static constexpr f32 DroneHoverAmpY = 0.035f; // metres of vertical bob
    static constexpr f32 DroneHoverFreqY = 1.7f;   // bob cycles per second
    static constexpr i32 DronePathSubdivisions = 4; // Catmull-Rom samples per segment

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

    void Entity_DroneQuad::Think() {
        WaypointGraph & navGraph = map->GetNavGraph();
        const i32 nodeCount = navGraph.GetNodeCount();
        if ( nodeCount < 2 ) {
            return;
        }

        // Compute a new A* path to a random node
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

        // Collect raw waypoint positions
        std::vector<Vec3> rawPoints;
        rawPoints.reserve( path.size() );
        for ( i32 idx : path ) {
            rawPoints.push_back( navGraph.GetWaypoint( idx ).position );
        }

        // Build Catmull-Rom smoothed path
        smoothedPath.clear();
        const i32 count = (i32)rawPoints.size();
        for ( i32 i = 0; i < count - 1; i++ ) {
            // For endpoints, reflect to create phantom control points
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

        // Start following from the first point beyond our current position
        smoothedPathIndex = 1;
        moveTarget = smoothedPath[ smoothedPathIndex ];
        hasTarget = true;
    }

    void Entity_DroneQuad::OnUpdate( f32 dt ) {
        hoverTime += dt;

        if ( !hasTarget ) {
            Think();
        }

        // ---- Movement / arrival ----
        if ( hasTarget ) {
            Vec3 toTarget = moveTarget - basePosition;
            f32  dist = Length( toTarget );
            bool isFinalTarget = smoothedPathIndex >= (i32)smoothedPath.size();

            if ( isFinalTarget && dist < DroneArrivalDist ) {
                // Final destination: stop
                basePosition = moveTarget;
                velocity = Vec3( 0.0f );
                hasTarget = false;
            }
            else if ( !isFinalTarget && dist < DronePassThroughDist ) {
                // Intermediate point: advance to next without stopping
                moveTarget = smoothedPath[ smoothedPathIndex ];
                smoothedPathIndex++;
            }
            else {
                Vec3 dir = toTarget / dist;
                f32  desiredSpeed;
                if ( isFinalTarget ) {
                    // Brake only when approaching the final target
                    desiredSpeed = DroneMoveMaxSpeed * SmoothStep( 0.0f, DroneSlowdownDist, dist );
                }
                else {
                    // Cruise at full speed through intermediate points
                    desiredSpeed = DroneMoveMaxSpeed;
                }
                Vec3 desiredVel = dir * desiredSpeed;
                velocity = Lerp( velocity, desiredVel, Clamp( DroneMoveAccel * dt, 0.0f, 1.0f ) );
            }
        }
        else {
            // Dampen residual velocity after arrival
            velocity = Lerp( velocity, Vec3( 0.0f ), Clamp( DroneMoveAccel * dt, 0.0f, 1.0f ) );
        }

        basePosition += velocity * dt;

        // ---- Yaw: face direction of travel ----
        Vec2 horizVel = Vec2( velocity.x, velocity.z );
        f32  horizSpeed = Length( horizVel );
        f32  yawRate = 0.0f;

        if ( horizSpeed > 0.1f ) {
            f32 targetYaw = atan2f( velocity.x, velocity.z );

            // Shortest-path difference
            f32 yawDiff = targetYaw - smoothYaw;
            while ( yawDiff > PI ) yawDiff -= TWO_PI;
            while ( yawDiff < -PI ) yawDiff += TWO_PI;

            f32 yawStep = Clamp( yawDiff, -DroneYawSpeed * dt, DroneYawSpeed * dt );
            yawRate = (dt > 0.0f) ? (yawStep / dt) : 0.0f;
            smoothYaw += yawStep;
        }

        // ---- Pitch: nose tilts opposite to forward acceleration ----
        Vec3 facing = Vec3( sinf( smoothYaw ), 0.0f, cosf( smoothYaw ) );
        f32  forwardSpd = Dot( velocity, facing );
        f32  targetPitch = -forwardSpd * DronePitchFactor;
        targetPitch += sinf( hoverTime * 0.89f + 1.3f ) * 0.006f;  // micro-correction wobble
        smoothPitch = Lerp( smoothPitch, targetPitch, Clamp( DronePitchLerpRate * dt, 0.0f, 1.0f ) );

        // ---- Roll: bank into turns (centripetal lean) ----
        f32 targetRoll = yawRate * horizSpeed * DroneBankFactor;
        targetRoll += sinf( hoverTime * 0.73f ) * 0.008f;            // micro-correction wobble
        smoothRoll = Lerp( smoothRoll, targetRoll, Clamp( DroneRollLerpRate * dt, 0.0f, 1.0f ) );

        // ---- Build orientation from stacked rotations ----
        Quat yawQ = glm::angleAxis( smoothYaw, Vec3( 0.0f, 1.0f, 0.0f ) );
        Quat pitchQ = glm::angleAxis( smoothPitch, Vec3( 1.0f, 0.0f, 0.0f ) );
        Quat rollQ = glm::angleAxis( smoothRoll, Vec3( 0.0f, 0.0f, 1.0f ) );
        orientation = Mat3( yawQ * pitchQ * rollQ );

        // ---- Hover bob (applied last, purely visual) ----
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

    void Entity_DroneQuad::TakeDamage( i32 damage ) {
        health -= damage;
        if ( health <= 0 ) {
            map->DestroyEntity( this );
        }
    }

    Entity_GameMode_KillAllEntities::Entity_GameMode_KillAllEntities() {
        type = EntityType::GameMode_KillAllEntities;
    }

    void Entity_GameMode_KillAllEntities::OnSpawn() {

    }

    void Entity_GameMode_KillAllEntities::OnUpdate( f32 dt ) {
        for ( i32 spawnIdIndex = 0; spawnIdIndex < (i32)remainingEntities.size(); spawnIdIndex++ ) {
            bool missing = true;
            for ( i32 entityIndex = 0; entityIndex < map->GetEntityCount(); entityIndex++ ) {
                const Entity * entity = map->GetEntity( entityIndex );
                if ( entity->GetSpawnId() == remainingEntities[spawnIdIndex] ) {
                    missing = false;
                    continue;
                }
            }

            if ( missing == true ) {
                LOG_INFO( "Entity_GameMode_KillAllEntities :: Removing :: %d", remainingEntities[spawnIdIndex] );
                remainingEntities.erase( remainingEntities.begin() + spawnIdIndex );
                spawnIdIndex--;
            }
        }

        if ( remainingEntities.size() == 0 ) {
            Engine::Get().TransitionToScene( "GameMapScene", mapName.c_str() );
            LOG_INFO( "Entity_GameMode_KillAllEntities :: Game over" );
        }
    }

    void Entity_GameMode_KillAllEntities::OnRender( Renderer & renderer ) {

    }

    void Entity_GameMode_KillAllEntities::OnDespawn() {
    }

    void Entity_GameMode_KillAllEntities::Serialize( Serializer & serializer ) {
        Entity::Serialize( serializer );
        serializer( "MapName", mapName );
        serializer( "RemainingEntities", remainingEntities );
    }
}