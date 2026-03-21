#include "editor_asset_thumbnail.h"

#include "../engine/atto_engine.h"

#include <glad/glad.h>
#include <filesystem>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write/stb_image_write.h"

namespace atto {

    ThumbnailBaker::ThumbnailBaker() {
        // -- MSAA framebuffer (render target) --
        glGenFramebuffers( 1, &msaaFbo );
        glBindFramebuffer( GL_FRAMEBUFFER, msaaFbo );

        glGenRenderbuffers( 1, &msaaColorRbo );
        glBindRenderbuffer( GL_RENDERBUFFER, msaaColorRbo );
        glRenderbufferStorageMultisample( GL_RENDERBUFFER, MSAA_SAMPLES, GL_RGBA8, THUMBNAIL_SIZE, THUMBNAIL_SIZE );
        glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, msaaColorRbo );

        glGenRenderbuffers( 1, &msaaDepthRbo );
        glBindRenderbuffer( GL_RENDERBUFFER, msaaDepthRbo );
        glRenderbufferStorageMultisample( GL_RENDERBUFFER, MSAA_SAMPLES, GL_DEPTH24_STENCIL8, THUMBNAIL_SIZE, THUMBNAIL_SIZE );
        glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, msaaDepthRbo );

        ATTO_ASSERT( glCheckFramebufferStatus( GL_FRAMEBUFFER ) == GL_FRAMEBUFFER_COMPLETE, "" );

        // -- Resolve framebuffer (non-MSAA, for readback) --
        glGenFramebuffers( 1, &resolveFbo );
        glBindFramebuffer( GL_FRAMEBUFFER, resolveFbo );

        glGenTextures( 1, &resolveColorTex );
        glBindTexture( GL_TEXTURE_2D, resolveColorTex );
        glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8, THUMBNAIL_SIZE, THUMBNAIL_SIZE, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
        glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, resolveColorTex, 0 );

        ATTO_ASSERT( glCheckFramebufferStatus( GL_FRAMEBUFFER ) == GL_FRAMEBUFFER_COMPLETE, "" );

        glBindFramebuffer( GL_FRAMEBUFFER, 0 );
    }

    ThumbnailBaker::~ThumbnailBaker() {
        glDeleteTextures( 1, &resolveColorTex );
        glDeleteFramebuffers( 1, &resolveFbo );
        glDeleteRenderbuffers( 1, &msaaDepthRbo );
        glDeleteRenderbuffers( 1, &msaaColorRbo );
        glDeleteFramebuffers( 1, &msaaFbo );
    }

    void ThumbnailBaker::GenerateThumbnailsForFolder( const char * folderPath ) {
        AssetManager & assetManager = Engine::Get().GetAssetManager();
        Logger & logger = Logger::Get();

        std::vector<std::string> files = assetManager.GetFilesInFolderRecursive( folderPath, ".obj" );
        const int fileCount = (int)files.size();
        int currentFile = 1;
        for ( const std::string & file : files ) {
            logger.SetMinLevel( LogLevel::Warn );
            GenerateThumbnailForModel( file.c_str() );
            logger.SetMinLevel( LogLevel::Trace );
            ATTO_LOG_TRACE("Baked [ %d / %d ] -> %s ", currentFile, fileCount, file.c_str() );
            currentFile++;
        }
    }

    void ThumbnailBaker::GenerateThumbnailForModel( const char * modelPath ) {
        LOG_INFO( "Generating thumbnail for %s", modelPath );
        StaticModel model;
        model.LoadFromFile( modelPath );
        GenerateThumbnailForModel( &model );
        model.Destroy();
    }

    void ThumbnailBaker::GenerateThumbnailForModel( const StaticModel * model ) {
        if ( !model || !model->IsLoaded() ) {
            return;
        }

        Renderer & renderer = Engine::Get().GetRenderer();

        // -- Save current GL state --
        GLint prevViewport[4];
        glGetIntegerv( GL_VIEWPORT, prevViewport );

        GLint prevFbo = 0;
        glGetIntegerv( GL_FRAMEBUFFER_BINDING, &prevFbo );

        // -- Set up camera looking from top-right --
        AlignedBox bounds = model->GetBounds();
        Vec3 center = bounds.GetCenter();
        Vec3 size = bounds.GetSize();
        f32 extent = Max( Max( size.x, size.y ), size.z );

        // Camera offset: top-right-front (positive X, positive Y, positive Z)
        Vec3 cameraDir = Normalize( Vec3( 1.0f, 0.8f, 1.0f ) );
        f32 distance = extent * 1.55f;
        Vec3 cameraPos = center + cameraDir * distance;

        Mat4 view = glm::lookAt( cameraPos, center, Vec3( 0.0f, 1.0f, 0.0f ) );
        Mat4 proj = glm::perspective( ToRadians( 45.0f ), 1.0f, 0.01f, distance * 4.0f );
        Mat4 vp = proj * view;

        // -- Render to MSAA FBO --
        glBindFramebuffer( GL_FRAMEBUFFER, msaaFbo );
        glViewport( 0, 0, THUMBNAIL_SIZE, THUMBNAIL_SIZE );

        glClearColor( 0.8f, 0.8f, 0.8f, 1.0f );
        glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
        glEnable( GL_DEPTH_TEST );

        renderer.SetViewProjectionMatrix( vp );
        renderer.UseLitShader();
        renderer.RenderStaticModel( model, Mat4( 1.0f ) );

        // -- Resolve MSAA to single-sample FBO --
        glBindFramebuffer( GL_READ_FRAMEBUFFER, msaaFbo );
        glBindFramebuffer( GL_DRAW_FRAMEBUFFER, resolveFbo );
        glBlitFramebuffer( 0, 0, THUMBNAIL_SIZE, THUMBNAIL_SIZE,
            0, 0, THUMBNAIL_SIZE, THUMBNAIL_SIZE,
            GL_COLOR_BUFFER_BIT, GL_LINEAR );

        // -- Read back pixels from resolved FBO --
        glBindFramebuffer( GL_FRAMEBUFFER, resolveFbo );

        std::vector<u8> pixels( THUMBNAIL_SIZE * THUMBNAIL_SIZE * 4 );
        glReadPixels( 0, 0, THUMBNAIL_SIZE, THUMBNAIL_SIZE, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data() );

        // -- Flip vertically (OpenGL reads bottom-to-top, PNG expects top-to-bottom) --
        const i32 rowBytes = THUMBNAIL_SIZE * 4;
        std::vector<u8> rowTemp( rowBytes );
        for ( i32 y = 0; y < THUMBNAIL_SIZE / 2; y++ ) {
            u8 * top = pixels.data() + y * rowBytes;
            u8 * bot = pixels.data() + (THUMBNAIL_SIZE - 1 - y) * rowBytes;
            memcpy( rowTemp.data(), top, rowBytes );
            memcpy( top, bot, rowBytes );
            memcpy( bot, rowTemp.data(), rowBytes );
        }

        // -- Restore previous GL state --
        glBindFramebuffer( GL_FRAMEBUFFER, (GLuint)prevFbo );
        glViewport( prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3] );

        // -- Build output path: path/ModelName.png --
        std::filesystem::path outDir( path );
        std::filesystem::create_directories( outDir );

        std::string modelName = model->GetPath().GetCStr();
        std::filesystem::path modelFilePath( modelName );
        std::string stem = modelFilePath.stem().string();
        std::filesystem::path outPath = outDir / (stem + ".png");

        stbi_write_png( outPath.string().c_str(), THUMBNAIL_SIZE, THUMBNAIL_SIZE, 4, pixels.data(), rowBytes );
    }

}
