#include "game_map.h"

namespace atto {

    GameMap::GameMap() {
    }

    GameMap::~GameMap() {
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
    }

    void GameMap::Serialize( Serializer & serializer ) {

    }
}


