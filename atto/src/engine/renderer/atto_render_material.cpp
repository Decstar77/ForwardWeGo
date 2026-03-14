#include "atto_render_material.h"
#include "../atto_log.h"

#include <glad/glad.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image/std_image.h>

namespace atto {
    void Texture::LoadFromFile( const char * filePath ) {
        int width, height, channels;
        stbi_uc * data = stbi_load( filePath, &width, &height, &channels, STBI_rgb_alpha );
        if ( !data ) {
            LOG_ERROR( "Failed to load texture image '%s'", filePath );
            width = height = 0;
            texture = 0;
            return;
        }

        this->width = width;
        this->height = height;

        glGenTextures( 1, &texture );
        glBindTexture( GL_TEXTURE_2D, texture );

        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

        glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data );
        glGenerateMipmap( GL_TEXTURE_2D );

        glBindTexture( GL_TEXTURE_2D, 0 );

        stbi_image_free( data );
    }

}