#include "engine/atto_engine.h"
#include "engine/atto_assets.h"

#include "entities_def.h"

namespace atto {

    constexpr f32 PlayerHeight = 2.0f;
    constexpr f32 PlayerEyeHeight = 1.8f;

    struct PlayerStart {
        Vec3 spawnPos;
        Mat3 spawnOri;

        void Serialize( Serializer & serializer );

        inline Capsule GetCapsule() const { return Capsule::FromTips( spawnPos, Vec3( spawnPos.x, spawnPos.y + PlayerHeight, spawnPos.z ), 0.4f ); }
    };

    struct MapRaycastResult {
        Entity *    entity;
        i32         brushIndex;
        f32         distance;
    };

    class GameMap {
    public:
        GameMap();
        ~GameMap();

        void Initialize();

        void StartMap();

        void Update( f32 dt );
        void Render( Renderer & renderer, f32 dt, i32 selectedBrush = -1 );

        void Serialize( Serializer & serializer );

        // =========== Not sure ===========
        bool Raycast( const Vec3 & start, const Vec3 & direction, MapRaycastResult & result );

        // =========== Player ===========
        PlayerStart &        GetPlayerStart() { return playerStart; }
        const PlayerStart &  GetPlayerStart() const { return playerStart; }
        bool                 IsPlayerStartColliding() const;

        // =========== Entities ===========
        std::unique_ptr<Entity> MakeEntity( EntityType type );
        Entity *        CreateEntity( EntityType type );
        void            DestroyEntity( Entity * entity );
        void            DestroyEntityByIndex( i32 index );
        Entity *        GetEntity( i32 index ) { return entities[index].get(); }
        const Entity *  GetEntity( i32 index ) const { return entities[index].get(); }
        i32             GetEntityCount() const { return static_cast<i32>(entities.size()); }

        // =========== Brushes ===========
        i32             AddBrush();
        void            RemoveBrush( i32 index );
        void            RebuildBrushModel( i32 index );
        void            RebuildBrushCollision( i32 index );
        void            RebuildAllBrushModels();
        void            RebuildAllBrushCollision();
        void            DebugDrawBrushCollision( Renderer & renderer ) const;
        Vec3            ResolvePlayerCollision( const Capsule & playerCapsule ) const;
        Brush &         GetBrush( i32 index ) { return brushes[index]; }
        const Brush &   GetBrush( i32 index ) const { return brushes[index]; }
        i32             GetBrushCount() const { return static_cast<i32>(brushes.size()); }

    private:
        PlayerStart playerStart;

        std::vector<std::unique_ptr<Entity>>    entities;
        std::vector<Brush>                      brushes;
        std::vector<StaticModel>                brushModels;
        std::vector<AlignedBox>                 brushCollsion;
    };
}
