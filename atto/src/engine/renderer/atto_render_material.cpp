#include "atto_render_material.h"
#include "../atto_log.h"
#include "../atto_engine.h"
#include "../atto_assets.h"

#include <glad/glad.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image/std_image.h>

namespace atto {

    // =========================================================================
    // Texture
    // =========================================================================

    void Texture::LoadFromFile( const char * filePath ) {
        Destroy();

        int w, h, channels;
        stbi_uc * data = stbi_load( filePath, &w, &h, &channels, STBI_rgb_alpha );
        if ( !data ) {
            LOG_ERROR( "Failed to load texture image '%s'", filePath );
            return;
        }

        width = w;
        height = h;

        glGenTextures( 1, &handle );
        glBindTexture( GL_TEXTURE_2D, handle );

        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

        glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data );
        glGenerateMipmap( GL_TEXTURE_2D );

        glBindTexture( GL_TEXTURE_2D, 0 );

        stbi_image_free( data );

        path = SmallString::FromLiteral( filePath );
    }

    void Texture::Destroy() {
        if ( handle != 0 ) {
            glDeleteTextures( 1, &handle );
            handle = 0;
        }
        width = 0;
        height = 0;
    }

    void Texture::Bind( i32 slot ) const {
        glActiveTexture( GL_TEXTURE0 + slot );
        glBindTexture( GL_TEXTURE_2D, handle );
    }

    void Texture::Unbind( i32 slot ) const {
        glActiveTexture( GL_TEXTURE0 + slot );
        glBindTexture( GL_TEXTURE_2D, 0 );
    }

    // =========================================================================
    // Shader
    // =========================================================================

    u32 Shader::CompileStage( u32 type, const char * source ) {
        u32 shader = glCreateShader( type );
        glShaderSource( shader, 1, &source, nullptr );
        glCompileShader( shader );

        i32 success = 0;
        glGetShaderiv( shader, GL_COMPILE_STATUS, &success );
        if ( !success ) {
            char infoLog[512];
            glGetShaderInfoLog( shader, sizeof( infoLog ), nullptr, infoLog );
            const char * typeStr = (type == GL_VERTEX_SHADER) ? "VERTEX" : "FRAGMENT";
            LOG_ERROR( "Shader compilation failed (%s): %s", typeStr, infoLog );
            glDeleteShader( shader );
            return 0;
        }

        return shader;
    }

    bool Shader::CreateFromFiles( const char * vertexPath, const char * fragmentPath ) {
        std::string vertSrc = Engine::Get().GetAssetManager().ReadTextFile( vertexPath );
        if ( vertSrc.empty() ) {
            LOG_ERROR( "Failed to read vertex shader file: %s", vertexPath );
            return false;
        }

        std::string fragSrc = Engine::Get().GetAssetManager().ReadTextFile( fragmentPath );
        if ( fragSrc.empty() ) {
            LOG_ERROR( "Failed to read fragment shader file: %s", fragmentPath );
            return false;
        }

        return CreateFromSource( vertSrc.c_str(), fragSrc.c_str() );
    }

    bool Shader::CreateFromSource( const char * vertexSrc, const char * fragmentSrc ) {
        Destroy();

        u32 vertShader = CompileStage( GL_VERTEX_SHADER, vertexSrc );
        if ( vertShader == 0 ) return false;

        u32 fragShader = CompileStage( GL_FRAGMENT_SHADER, fragmentSrc );
        if ( fragShader == 0 ) {
            glDeleteShader( vertShader );
            return false;
        }

        u32 program = glCreateProgram();
        glAttachShader( program, vertShader );
        glAttachShader( program, fragShader );
        glLinkProgram( program );

        i32 success = 0;
        glGetProgramiv( program, GL_LINK_STATUS, &success );
        if ( !success ) {
            char infoLog[512];
            glGetProgramInfoLog( program, sizeof( infoLog ), nullptr, infoLog );
            LOG_ERROR( "Shader program linking failed: %s", infoLog );
            glDeleteProgram( program );
            program = 0;
        }

        glDeleteShader( vertShader );
        glDeleteShader( fragShader );

        programHandle = program;
        return programHandle != 0;
    }

    void Shader::Destroy() {
        if ( programHandle != 0 ) {
            glDeleteProgram( programHandle );
            programHandle = 0;
        }
    }

    void Shader::Bind() const {
        glUseProgram( programHandle );
    }

    void Shader::Unbind() const {
        glUseProgram( 0 );
    }

    i32 Shader::GetUniformLocation( const char * name ) const {
        return glGetUniformLocation( programHandle, name );
    }

    // By location
    void Shader::SetInt( i32 location, i32 value ) const {
        glUniform1i( location, value );
    }

    void Shader::SetFloat( i32 location, f32 value ) const {
        glUniform1f( location, value );
    }

    void Shader::SetVec2( i32 location, const Vec2 & value ) const {
        glUniform2fv( location, 1, &value.x );
    }

    void Shader::SetVec3( i32 location, const Vec3 & value ) const {
        glUniform3fv( location, 1, &value.x );
    }

    void Shader::SetVec4( i32 location, const Vec4 & value ) const {
        glUniform4fv( location, 1, &value.x );
    }

    void Shader::SetMat3( i32 location, const Mat3 & value ) const {
        glUniformMatrix3fv( location, 1, GL_FALSE, &value[0][0] );
    }

    void Shader::SetMat4( i32 location, const Mat4 & value ) const {
        glUniformMatrix4fv( location, 1, GL_FALSE, &value[0][0] );
    }

    void Shader::SetMat4Array( i32 location, const Mat4 * values, i32 count ) const {
        glUniformMatrix4fv( location, count, GL_FALSE, &values[0][0][0] );
    }

    // By name (convenience, does a lookup each call)
    void Shader::SetInt( const char * name, i32 value ) const {
        SetInt( GetUniformLocation( name ), value );
    }

    void Shader::SetFloat( const char * name, f32 value ) const {
        SetFloat( GetUniformLocation( name ), value );
    }

    void Shader::SetVec2( const char * name, const Vec2 & value ) const {
        SetVec2( GetUniformLocation( name ), value );
    }

    void Shader::SetVec3( const char * name, const Vec3 & value ) const {
        SetVec3( GetUniformLocation( name ), value );
    }

    void Shader::SetVec4( const char * name, const Vec4 & value ) const {
        SetVec4( GetUniformLocation( name ), value );
    }

    void Shader::SetMat3( const char * name, const Mat3 & value ) const {
        SetMat3( GetUniformLocation( name ), value );
    }

    void Shader::SetMat4( const char * name, const Mat4 & value ) const {
        SetMat4( GetUniformLocation( name ), value );
    }

    void Shader::SetMat4Array( const char * name, const Mat4 * values, i32 count ) const {
        SetMat4Array( GetUniformLocation( name ), values, count );
    }

}
