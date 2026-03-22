#include "game_map.h"

#include "entities/game_entity_roach.h"

namespace atto {

    void PlayerStart::Serialize( Serializer & serializer ) {
        serializer( "spawnPos", spawnPos );
        serializer( "spawnOri", spawnOri );
    }

    GameMap::GameMap() {
        playerStart.spawnPos = Vec3( 0.0f, 0.0f, 3.0f );
        playerStart.spawnOri = Mat3( 1 );
    }

    GameMap::~GameMap() {
        for ( auto & bm : brushModels ) {
            bm.Destroy();
        }
    }

    void GameMap::Clear() {
        for ( auto & bm : brushModels ) {
            bm.Destroy();
        }
        brushModels.clear();
        brushTextures.clear();
        brushCollsion.clear();
        brushes.clear();

        for ( auto & entity : entities ) {
            entity->OnDespawn();
        }
        entities.clear();

        playerStart.spawnPos = Vec3( 0.0f, 0.0f, 3.0f );
        playerStart.spawnOri = Mat3( 1 );

        navGraph.Clear();
    }

    void GameMap::Initialize() {
        brushModels.resize( brushes.size() );
        brushTextures.resize( brushes.size(), nullptr );
        brushCollsion.resize( brushes.size() );
        RebuildAllBrushModels();
        RebuildAllBrushCollision();
        RebuildAllBrushTextures();

        for ( auto & entity : entities ) {
            entity->OnSpawn();
        }
    }

    void GameMap::StartMap() {

    }

    void GameMap::Update( f32 dt ) {
        for ( auto & entity : entities ) {
            if ( !entity->IsPendingDestroy() ) {
                entity->OnUpdate( dt );
            }
        }

        FlushDestroyedEntities();
    }

    void GameMap::FlushDestroyedEntities() {
        for ( i32 i = static_cast<i32>( entities.size() ) - 1; i >= 0; i-- ) {
            if ( entities[i]->IsPendingDestroy() ) {
                entities[i]->OnDespawn();
                entities.erase( entities.begin() + i );
            }
        }
    }

    void GameMap::Render( Renderer & renderer, f32 dt, i32 selectedBrush ) {
        Mat4 identity( 1.0f );
        for ( i32 i = 0; i < static_cast<i32>( brushModels.size() ); i++ ) {
            if ( !brushModels[i].IsLoaded() ) {
                continue;
            }
            Vec3 color = (i == selectedBrush) ? Vec3( 0.2f, 0.8f, 0.2f ) : Vec3( 1.0f, 1.0f, 1.0f );
            for ( i32 j = 0; j < brushModels[i].GetMeshCount(); j++ ) {
                brushModels[i].GetMesh( j ).GetMaterial().albedo = color;
                brushModels[i].GetMesh( j ).GetMaterial().albedoTexture = brushTextures[i];
            }
            renderer.RenderStaticModel( &brushModels[i], identity, color );
        }

        for ( auto & entity : entities ) {
            entity->OnRender( renderer );

            if ( Engine::Get().GetInput().IsKeyDown( Key::Num5 )) {
                entity->DebugDrawBounds( renderer );
            }
            if ( Engine::Get().GetInput().IsKeyDown( Key::Num6 )) {
                entity->DebugDrawCollider( renderer );
            }
        }
    }

    std::unique_ptr<Entity> GameMap::MakeEntity( EntityType type ) {
        ClassFactory<Entity> factory;
        std::unique_ptr<Entity> entity = factory.CreateUnique( static_cast<i64>( type ) );
        ATTO_ASSERT( entity != nullptr, "Made entity is null" );
        ATTO_ASSERT( entity->GetType() != EntityType::None, "Made entity with None type" );
        return entity;
    }

    Entity * GameMap::CreateEntity( EntityType type ) {
        auto entity = MakeEntity( type );
        if ( !entity ) return nullptr;
        entity->SetMap( this );
        entity->SetSpawnId( Engine::Get().GetRNG().Unsigned64() );
        return entities.emplace_back( std::move( entity ) ).get();
    }

    void GameMap::DestroyEntity( Entity * entity ) {
        if ( entity ) {
            entity->MarkForDestroy();
        }
    }

    void GameMap::DestroyEntityByIndex( i32 index ) {
        if ( index < 0 || index >= static_cast<i32>( entities.size() ) ) {
            return;
        }
        entities[index]->MarkForDestroy();
    }

    i32 GameMap::AddBrush() {
        Brush brush;
        brushes.push_back( brush );
        brushModels.emplace_back();
        brushTextures.push_back( nullptr );
        brushCollsion.emplace_back();
        i32 index = static_cast<i32>(brushes.size()) - 1;
        RebuildBrushModel( index );
        return index;
    }

    void GameMap::RemoveBrush( i32 index ) {
        if ( index < 0 || index >= static_cast<i32>( brushes.size() ) ) {
            return;
        }
        brushModels[index].Destroy();
        brushes.erase( brushes.begin() + index );
        brushModels.erase( brushModels.begin() + index );
        brushTextures.erase( brushTextures.begin() + index );
        brushCollsion.erase( brushCollsion.begin() + index );
    }

    void GameMap::RebuildBrushModel( i32 index ) {
        if ( index < 0 || index >= static_cast<i32>( brushes.size() ) ) {
            return;
        }
        brushModels[index].Destroy();
        brushes[index].ToStaticModel( brushModels[index] );
    }

    void GameMap::RebuildBrushCollision( i32 index ) {
        if ( index < 0 || index >= static_cast<i32>( brushes.size() ) ) {
            return;
        }

        brushCollsion[index] = AlignedBox::FromCenterSize( brushes[index].center, brushes[index].halfExtents * 2.0f );
    }

    void GameMap::RebuildBrushTexture( i32 index ) {
        if ( index < 0 || index >= static_cast<i32>( brushes.size() ) ) {
            return;
        }

        if ( !brushes[index].texturePath.empty() ) {
            brushTextures[index] = Engine::Get().GetRenderer().GetOrLoadTexture( brushes[index].texturePath.c_str() );
        }
        else {
            brushTextures[index] = nullptr;
        }
    }

    void GameMap::RebuildAllBrushModels() {
        for ( i32 i = 0; i < static_cast<i32>( brushes.size() ); i++ ) {
            RebuildBrushModel( i );
        }
    }

    void GameMap::RebuildAllBrushCollision() {
        for ( i32 i = 0; i < static_cast<i32>( brushes.size() ); i++ ) {
            RebuildBrushCollision( i );
        }
    }

    void GameMap::RebuildAllBrushTextures() {
        for ( i32 i = 0; i < static_cast<i32>( brushes.size() ); i++ ) {
            RebuildBrushTexture( i );
        }
    }

    void GameMap::DebugDrawBrushCollision( Renderer & renderer ) const {
        for ( i32 i = 0; i < static_cast<i32>( brushes.size() ); i++ ) {
            renderer.DebugAlignedBox( brushCollsion[i] );
        }
    }

    Vec3 GameMap::ResolvePlayerCollision( const Capsule & playerCapsule ) const {
        Vec3 correction( 0.0f );
        Capsule cap = playerCapsule;

        for ( i32 iter = 0; iter < 2; iter++ ) {
            for ( i32 i = 0; i < static_cast<i32>( entities.size() ); i++ ) {
                Entity * entity = entities[i].get();
                if ( entity ) {
                    AlignedBox bounds = entity->GetBounds();
                    SweepResult result;
                    if ( CollisionSweep::CapsuleAlignedBox( cap, bounds, result ) ) {
                        Vec3 push = result.normal * result.pen;
                        cap.base += push;
                        correction += push;
                    }
                }
            }
        }

        for ( i32 iter = 0; iter < 4; iter++ ) {
            for ( i32 i = 0; i < static_cast<i32>( brushCollsion.size() ); i++ ) {
                SweepResult result;
                if ( CollisionSweep::CapsuleAlignedBox( cap, brushCollsion[i], result ) ) {
                    Vec3 push = result.normal * result.pen;
                    cap.base += push;
                    correction += push;
                }
            }
        }

        return correction;
    }

    bool GameMap::Raycast( const Vec3 & start, const Vec3 & direction, MapRaycastResult & result ) {
        f32 minDistance = FLT_MAX;

        result.entity = nullptr;
        result.brushIndex = -1;
        result.distance = FLT_MAX;

        for ( i32 i = 0; i < static_cast<i32>( brushCollsion.size() ); i++ ) {
            f32 distance = FLT_MAX;
            if ( Raycast::TestAlignedBox( start, direction, brushCollsion[i], distance ) ) {
                if ( distance < minDistance ) {
                    minDistance = distance;
                    result.entity = nullptr;
                    result.brushIndex = i;
                }
            }
        }

        for ( i32 i = 0; i < static_cast<i32>( entities.size() ); i++ ) {
            Entity * entity = entities[i].get();
            if ( entity ) {
                AlignedBox bounds = entity->GetBounds();
                f32 distance = FLT_MAX;
                if ( Raycast::TestAlignedBox( start, direction, bounds, distance ) ) {
                    if ( distance < minDistance ) {
                        minDistance = distance;
                        result.entity = entity;
                        result.brushIndex = -1;
                    }
                }
            }
        }

        result.distance = minDistance;
        return minDistance != FLT_MAX;
    }

    bool GameMap::IsPlayerStartColliding() const {
        Capsule cap = playerStart.GetCapsule();
        for ( i32 i = 0; i < static_cast<i32>( brushes.size() ); i++ ) {
            AlignedBox box = AlignedBox::FromCenterSize( brushes[i].center, brushes[i].halfExtents * 2.0f );
            if ( IntersectionTest::CapsuleAlignedBox( cap, box ) ) {
                return true;
            }
        }
        return false;
    }

    void GameMap::Serialize( Serializer & serializer ) {
        serializer( "playerStart", playerStart );
        serializer( "brushes", brushes );
        serializer( "navGraph", navGraph );

        serializer( "entities", entities, [this]( Serializer & sub ) -> std::unique_ptr<Entity>
            {
                std::string typeStr;
                sub( "Type", typeStr );
                EntityType entityType = StringToEntityType( typeStr.c_str() );

                auto entity = MakeEntity( entityType );
                if ( entity ) {
                    entity->SetMap( this );
                    entity->Serialize( sub );
                }
                return entity;
            } );
    }
}
