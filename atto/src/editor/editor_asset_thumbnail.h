#pragma once

#include "../engine/atto_core.h"

namespace atto {

    class StaticModel;

    class ThumbnailBaker {
    public:
        void GenerateThumbnailForModel( const char * modelPath );
        void GenerateThumbnailForModel( const StaticModel * model );

    private:
        std::string path = "assets/textures/editor/thumbnails";
    };

}
