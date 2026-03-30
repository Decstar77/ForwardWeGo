#pragma once
#include "engine/atto_engine.h"
#include "engine/atto_assets.h"
#include "engine/atto_waypoint_graph.h"

#include "game_player_controller.h"
#include "game_entities.h"

namespace atto {

    struct DamageNumber {
        Vec3    worldPos;
        i32     damage;
        f32     timer;
        f32     lifetime;
        f32     horizontalDir; // -1 to 1, random drift direction
    };

    struct MapRaycastResult {
        Entity *    entity;
        i32         brushIndex;
        f32         distance;
        Vec3        normal;
    };

    class GameMap {
    public:
        GameMap();
        ~GameMap();

        void Clear();
        void Initialize();

        void StartMap();
        void Shutdown();

        void Update( f32 dt );
        void Render( Renderer & renderer, f32 dt, i32 selectedBrush = -1, bool editorMode = false );

        void Serialize( Serializer & serializer );

        // =========== Not sure ===========
        const char * GetPath() const { return path.GetCStr(); }
        void SetPath( const char * path ) { this->path = LargeString::FromLiteral( path ); }
        bool Raycast( const Vec3 & start, const Vec3 & direction, MapRaycastResult & result );

        // =========== Player ===========
        PlayerStart &        GetPlayerStart() { return playerStart; }
        const PlayerStart &  GetPlayerStart() const { return playerStart; }
        bool                 IsPlayerStartColliding() const;

        void                 SetPlayerPosition( const Vec3 & pos ) { playerPosition = pos; }
        void                 SetPlayerCameraUp( const Vec3 & up ) { playerCameraUp = up; }
        Vec3                 GetPlayerPosition() const { return playerPosition; }
        Vec3                 GetPlayerCameraUp() const { return playerCameraUp; }
        void                 DamagePlayer( i32 damage ) { playerDamagePending += damage; }
        i32                  FlushPlayerDamage() { i32 d = playerDamagePending; playerDamagePending = 0; return d; }
        void                 HealPlayer( i32 amount ) { playerHealPending += amount; }
        i32                  FlushPlayerHeal() { i32 h = playerHealPending; playerHealPending = 0; return h; }

        // =========== Entities ===========
        std::unique_ptr<Entity> MakeEntity( EntityType type );
        Entity *        CreateEntity( EntityType type );
        void            DestroyEntity( Entity * entity );
        void            DestroyEntityByIndex( i32 index );
        void            FlushDestroyedEntities();
        void            FlushPendingEntities();
        Entity *        GetEntity( i32 index ) { return entities[index].get(); }
        const Entity *  GetEntity( i32 index ) const { return entities[index].get(); }
        i32             GetEntityCount() const { return static_cast<i32>(entities.size()); }

        // =========== NavGraph ===========
        WaypointGraph & GetNavGraph() { return navGraph; }
        const WaypointGraph & GetNavGraph() const { return navGraph; }

        // =========== Damage Numbers ===========
        void SpawnDamageNumber( const Vec3 & worldPos, i32 damage );
        const std::vector<DamageNumber> & GetDamageNumbers() const { return damageNumbers; }

        // =========== Particles ===========
        ParticleSystem & GetParticleSystem() { return particleSystem; }
        const ParticleSystem & GetParticleSystem() const { return particleSystem; }

        // =========== Brushes ===========
        i32             AddBrush();
        void            RemoveBrush( i32 index );
        void            RebuildBrushModel( i32 index );
        void            RebuildBrushCollision( i32 index );
        void            RebuildBrushTexture( i32 index );
        void            RebuildAllBrushModels();
        void            RebuildAllBrushCollision();
        void            RebuildAllBrushTextures();
        void            DebugDrawBrushCollision( Renderer & renderer ) const;
        Vec3            ResolvePlayerCollision( const Capsule & playerCapsule ) const;
        Brush &         GetBrush( i32 index ) { return brushes[index]; }
        const Brush &   GetBrush( i32 index ) const { return brushes[index]; }
        i32             GetBrushCount() const { return static_cast<i32>(brushes.size()); }

    private:
        LargeString                             path;
        PlayerStart                             playerStart;

        Vec3                                    playerPosition = Vec3( 0.0f );
        Vec3                                    playerCameraUp = Vec3( 0.0f );
        i32                                     playerDamagePending = 0;
        i32                                     playerHealPending = 0;

        WaypointGraph                           navGraph;
        ParticleSystem                          particleSystem;

        std::vector<DamageNumber>               damageNumbers;
        std::vector<std::unique_ptr<Entity>>    entities;
        std::vector<std::unique_ptr<Entity>>    pendingEntities;
        std::vector<Brush>                      brushes;
        std::vector<StaticModel>                brushModels;
        std::vector<const Texture *>            brushTextures;
        std::vector<AlignedBox>                 brushCollsion;
    };
}
