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
        Prop,
        Roach,

        EntityTypeCount
    };

    typedef u64 SpawnId;

    inline static const i32 EntityTypeCount = static_cast<i32>(EntityType::EntityTypeCount);

    inline static const EntityType EntityTypes[] = {
        EntityType::None,
        EntityType::Barrel,
        EntityType::Drone_QUAD,
        EntityType::ExitDoor,
        EntityType::GameMode_KillAllEntities,
        EntityType::Prop,
        EntityType::Roach,
    };

    inline static const char * EntityTypeNames[] = {
        "None",
        "Barrel",
        "Drone_QUAD",
        "ExitDoor",
        "GameMode_KillAllEntities",
        "Prop",
        "Roach"
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
        virtual ~Entity() = default;

        // ================ Getters =============== //
        EntityType      GetType() const { return type; }
        void            SetPosition( const Vec3 & pos ) { position = pos; }
        void            SetOrientation( const Mat3 & orient ) { orientation = orient; }
        const Vec3 &    GetPosition() const { return position; }
        const Mat3 &    GetOrientation() const { return orientation; }
        void            SetMap( GameMap * map ) { this->map = map; }
        GameMap *       GetMap() const { return map; }
        SpawnId         GetSpawnId() const { return spawnId; }
        void            SetSpawnId( SpawnId id ) { spawnId = id; }
        bool            IsPendingDestroy() const { return pendingDestroy; }
        void            MarkForDestroy() { pendingDestroy = true; }

        Mat4            GetModelMatrix() const;

        // ================ Standard =============== //
        virtual void        OnSpawn() {}
        virtual void        OnUpdate( f32 dt ) {}
        virtual void        OnRender( Renderer & renderer ) {}
        virtual void        OnDespawn() {}

        // ================ Physics =============== //
        virtual AlignedBox  GetBounds() const { return {}; }
        virtual Box         GetCollider() const { return {}; }
        virtual bool        RayTest( const Vec3 & start, const Vec3 & dir, f32 & dist ) const { return false; }

        // ================ Serialization =============== //
        virtual void        Serialize( Serializer & serializer );

        // ================ Gameplay =============== //
        virtual void        TakeDamage( i32 damage ) {}

        // ================ Debug =============== //
        virtual void        DebugDrawBounds( Renderer & renderer ) {}
        virtual void        DebugDrawCollider( Renderer & renderer ) {};

    protected:
        EntityType type = EntityType::None;
        SpawnId spawnId = 0;
        GameMap * map = nullptr;
        Vec3 position = Vec3( 0.0f, 0.0f, 0.0f );
        Mat3 orientation = Mat3( 1 );
        bool pendingDestroy = false;
    };
}

