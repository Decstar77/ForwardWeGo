#include "game_map.h"

namespace atto {

    GameMap::GameMap() {
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
    }

    void GameMap::Update( f32 dt ) {
    }

    void GameMap::Render( Renderer & renderer, f32 dt, bool lit ) {
        if ( lit ) {
            renderer.RenderStaticModel( model, Mat4( 1.0f ) );
        }
        else {
            renderer.RenderStaticModelUnlit( model, Mat4( 1.0f ) );
        }

        Mat4 identity( 1.0f );
        for ( const auto & bm : brushModels ) {
            if ( !bm.IsLoaded() ) {
                continue;
            }
            if ( lit ) {
                renderer.RenderStaticModel( bm, identity );
            }
            else {
                renderer.RenderStaticModelUnlit( bm, identity );
            }
        }
    }

    i32 GameMap::AddBrush() {
        Brush brush;
        brushes.push_back( brush );
        brushModels.emplace_back();
        i32 index = static_cast<i32>( brushes.size() ) - 1;
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

    void GameMap::RebuildAllBrushModels() {
        for ( i32 i = 0; i < static_cast<i32>( brushes.size() ); i++ ) {
            RebuildBrushModel( i );
        }
    }

    void GameMap::Serialize( Serializer & serializer ) {
        serializer( "brushes", brushes );
    }
}
