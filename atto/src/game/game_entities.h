#pragma once
#include "engine/atto_engine.h"

namespace atto {

    class GameMap;

    enum class EntityType {
        None = 0,
        Barrel,
        Drone_QUAD,
        ExitDoor,
        GameMode_KillAllEntities,

        EntityTypeCount
    };

    typedef u64 SpawnId;

    inline static const i32 EntityTypeCount = static_cast<i32>(EntityType::EntityTypeCount);

    inline static const EntityType EntityTypes[] = {
        EntityType::None,
        EntityType::Barrel,
        EntityType::Drone_QUAD,
        EntityType::ExitDoor,
        EntityType::GameMode_KillAllEntities
    };

    inline static const char * EntityTypeNames[] = {
        "None",
        "Barrel",
        "Drone_QUAD",
        "ExitDoor",
        "GameMode_KillAllEntities"
    };

    static_assert(EntityTypeCount == sizeof( EntityTypes ) / sizeof( EntityTypes[0] ), "EntityTypes array size mismatch");
    static_assert(EntityTypeCount == sizeof( EntityTypeNames ) / sizeof( EntityTypeNames[0] ), "EntityTypeNames array size mismatch");

    inline const char * EntityTypeToString( EntityType type ) {
        return EntityTypeNames[static_cast<i32>(type)];
    }

    inline EntityType StringToEntityType( const char * str ) {
        for ( i32 i = 0; i < EntityTypeCount; i++ ) {
            if ( strcmp( str, EntityTypeNames[i] ) == 0 ) {
                return EntityTypes[i];
            }
        }
        return EntityType::None;
    }

    class Entity {
    public:
        // ================ Getters =============== //
        EntityType      GetType() const { return type; }
        void            SetPosition( const Vec3 & pos ) { position = pos; }
        void            SetOrientation( const Mat3 & orient ) { orientation = orient; }
        const Vec3 & GetPosition() const { return position; }
        const Mat3 & GetOrientation() const { return orientation; }
        void            SetMap( GameMap * map ) { this->map = map; }
        GameMap * GetMap() const { return map; }
        SpawnId         GetSpawnId() const { return spawnId; }
        void            SetSpawnId( SpawnId id ) { spawnId = id; }

        Mat4            GetModelMatrix() const;

        // ================ Standard =============== //
        virtual void        OnSpawn() {}
        virtual void        OnUpdate( f32 dt ) {}
        virtual void        OnRender( Renderer & renderer ) {}
        virtual void        OnDespawn() {}

        // ================ Physics =============== // 
        virtual AlignedBox  GetBounds() const { return {}; }
        virtual bool        RayTest( const Vec3 & start, const Vec3 & dir, f32 & dist ) const { return false; }

        // ================ Serialization =============== // 
        virtual void        Serialize( Serializer & serializer );

        // ================ Gameplay =============== // 
        virtual void        TakeDamage( i32 damage ) {}

        // ================ Debug =============== //
        virtual void        DebugDrawBounds( Renderer & renderer ) {}

    protected:
        EntityType type = EntityType::None;
        SpawnId spawnId = 0;
        GameMap * map = nullptr;
        Vec3 position = Vec3( 0.0f, 0.0f, 0.0f );
        Mat3 orientation = Mat3( 1 );
    };

    class Entity_Barrel : public Entity {
    public:
        Entity_Barrel();

        void OnSpawn() override;
        void OnUpdate( f32 dt ) override;
        void OnRender( Renderer & renderer ) override;
        void OnDespawn() override;

        AlignedBox GetBounds() const override;
        bool RayTest( const Vec3 & start, const Vec3 & dir, f32 & dist ) const override;
        void DebugDrawBounds( Renderer & renderer ) override;
        void TakeDamage( i32 damage ) override;

    private:
        const StaticModel * model = nullptr;
        i32 health = 100;
    };

    class Entity_ExitDoor : public Entity {
    public:
        Entity_ExitDoor();

        void OnSpawn() override;
        void OnUpdate( f32 dt ) override;
        void OnRender( Renderer & renderer ) override;
        void OnDespawn() override;

        AlignedBox GetBounds() const override;
        bool RayTest( const Vec3 & start, const Vec3 & dir, f32 & dist ) const override;
        void DebugDrawBounds( Renderer & renderer ) override;

    private:
        const StaticModel * modelClosed = nullptr;
        const StaticModel * modelOpen = nullptr;
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
        void TakeDamage( i32 damage ) override;

        void MoveTo( const Vec3 & target );

        void Serialize( Serializer & serializer ) override;

    private:
        void AdvanceWaypoint();

        const StaticModel * model = nullptr;
        i32 health = 100;

        // Waypoint patrol
        std::vector<Vec3> waypoints;
        i32  currentWaypointIndex = 0;

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
    };

    class Entity_GameMode_KillAllEntities : public Entity {
    public:
        Entity_GameMode_KillAllEntities();

        void OnSpawn() override;
        void OnUpdate( f32 dt ) override;
        void OnRender( Renderer & renderer ) override;
        void OnDespawn() override;

        void Serialize( Serializer & serializer ) override;

    private:
        std::string mapName = "";
        std::vector<SpawnId> remainingEntities;
    };
}