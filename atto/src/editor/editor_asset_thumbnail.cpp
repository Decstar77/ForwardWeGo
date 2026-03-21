#include "editor_asset_thumbnail.h"

#include "../engine/atto_engine.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

namespace atto {
    void GenerateThumbnailForModel( const char * modelPath ) {
        // Load the model directly without the use of the renderer so we can free it after.
        StaticModel model;
        model.LoadFromFile( modelPath );
        GenerateThumbnailForModel( &model );
        model.Destroy();
    }

    void GenerateThumbnailForModel( const StaticModel * model ) {
        Renderer & renderer = Engine::Get().GetRenderer();
        renderer.RenderStaticModel( model, Mat4( 1 ) );
    }

}