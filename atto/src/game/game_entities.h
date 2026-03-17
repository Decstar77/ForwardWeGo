#pragma once
#include "engine/atto_engine.h"

namespace atto {

    class GameMap;

    enum class EntityType {
        None = 0,
        Barrel = 1,
        Drone_QUAD = 2
    };

    constexpr EntityType EntityTypes[] = { EntityType::None, EntityType::Barrel, EntityType::Drone_QUAD };
    constexpr const char * EntityTypeNames[] = { "None", "Barrel", "Drone_QUAD" };
    constexpr i32 EntityTypeCount = static_cast<i32>( sizeof( EntityTypes ) / sizeof( EntityTypes[0] ) );

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

        // ================ Standard =============== //
        virtual void OnSpawn() {}
        virtual void OnUpdate( f32 dt ) {}
        virtual void OnRender( Renderer & renderer ) {}
        virtual void OnDespawn() {}

        // ================ Physics =============== // 
        virtual AlignedBox  GetBounds() const { return {}; }
        virtual bool        RayTest( const Vec3 & start, const Vec3 & dir, f32 & dist ) const { return false; }

        // ================ Serialization =============== // 
        virtual void        Serialize( Serializer & serializer );

        // ================ Gameplay =============== // 
        virtual void        TakeDamage( i32 damage ) {}

        // ================ Debug =============== //
        virtual void DebugDrawBounds( Renderer & renderer ) {}

    protected:
        EntityType type = EntityType::None;
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
        StaticModel model;
        i32 health = 100;
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

    public:
        StaticModel model;
        i32 health = 100;
    };
}