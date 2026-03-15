#pragma once
#include "engine/atto_engine.h"

namespace atto {

    class GameMap;

    enum class EntityType {
        None = 0,
        Barrel = 1
    };

    inline const char * EntityTypeToString( EntityType type ) {
        switch ( type ) {
        case EntityType::None:    return "None";
        case EntityType::Barrel:  return "Barrel";
        default:                  return "Unknown";
        }
    }

    inline EntityType StringToEntityType( const char * str ) {
        if ( strcmp( str, "None" ) == 0 ) return EntityType::None;
        if ( strcmp( str, "Barrel" ) == 0 ) return EntityType::Barrel;
        return EntityType::None;
    }

    class Entity {
    public:
        EntityType GetType() const { return type; }
        void SetPosition(const Vec3& pos) { position = pos; }
        void SetOrientation(const Mat3& orient) { orientation = orient; }
        const Vec3 & GetPosition() const { return position; }
        const Mat3 & GetOrientation() const { return orientation; }

        void SetMap( GameMap * map ) { this->map = map; }
        GameMap * GetMap() const { return map; }

        virtual void OnSpawn() {}
        virtual void OnUpdate( f32 dt ) {}
        virtual void OnRender( Renderer & renderer ) {}
        virtual void OnDespawn() {}

        virtual void Serialize( Serializer & serializer );

    protected:
        EntityType type;
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

    private:
        StaticModel model;
    };
}