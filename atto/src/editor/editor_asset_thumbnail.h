#pragma once

#include "../engine/atto_core.h"

namespace atto {

    class StaticModel;

    class ThumbnailBaker {
    public:
        ThumbnailBaker();
        ~ThumbnailBaker();

        void GenerateThumbnailsForFolder( const char * folderPath );
        void GenerateThumbnailForModel( const char * modelPath );
        void GenerateThumbnailForModel( const StaticModel * model );

    private:
        static constexpr i32 THUMBNAIL_SIZE = 256;
        static constexpr i32 MSAA_SAMPLES   = 8;

        std::string path = "assets/textures/editor/thumbnails";

        // MSAA framebuffer (render target)
        u32 msaaFbo      = 0;
        u32 msaaColorRbo = 0;
        u32 msaaDepthRbo = 0;

        // Resolve framebuffer (non-MSAA, for readback)
        u32 resolveFbo   = 0;
        u32 resolveColorTex = 0;
    };

}
