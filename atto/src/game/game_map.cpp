#include "game_map.h"

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
        model.Destroy();
    }

    void GameMap::Initialize() {
        model.LoadFromFile( "assets/sm/SM_Env_Tree_02.fbx", 0.01f );
        texture.LoadFromFile( "assets/PolygonScifi_01_C.png" );

        brushModels.resize( brushes.size() );
        brushCollsion.resize( brushes.size() );
        RebuildAllBrushModels();
        RebuildAllBrushCollision();
    }

    void GameMap::StartMap() {

    }

    void GameMap::Update( f32 dt ) {
    }

    void GameMap::Render( Renderer & renderer, f32 dt, bool lit, i32 selectedBrush ) {
        if ( lit ) {
            renderer.RenderStaticModel( model, Mat4( 1.0f ) );
        }
        else {
            renderer.RenderStaticModelUnlit( model, Mat4( 1.0f ) );
        }

        Mat4 identity( 1.0f );
        for ( i32 i = 0; i < static_cast<i32>( brushModels.size() ); i++ ) {
            if ( !brushModels[i].IsLoaded() ) {
                continue;
            }
            Vec3 color = (i == selectedBrush) ? Vec3( 0.2f, 0.8f, 0.2f ) : Vec3( 0.8f, 0.8f, 0.8f );
            if ( lit ) {
                renderer.RenderStaticModel( brushModels[i], identity, color );
            }
            else {
                renderer.RenderStaticModelUnlit( brushModels[i], identity, color );
            }
        }
    }

    i32 GameMap::AddBrush() {
        Brush brush;
        brushes.push_back( brush );
        brushModels.emplace_back();
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

    void GameMap::DebugDrawBrushCollision( Renderer & renderer ) const {
        for ( i32 i = 0; i < static_cast<i32>( brushes.size() ); i++ ) {
            renderer.DebugAlignedBox( brushCollsion[i] );
        }
    }

    void GameMap::Serialize( Serializer & serializer ) {
        serializer( "playerStart", playerStart );
        serializer( "brushes", brushes );
    }
}
