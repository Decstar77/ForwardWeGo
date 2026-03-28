#pragma once

#include "../atto_core.h"
#include "../atto_math.h"
#include "../atto_containers.h"

namespace atto {

    class Serializer;

    enum class TextureWrapType {
        CLAMP,
        REPEAT
    };

    struct TextureCreateInfo {
        bool                flip = false;
        TextureWrapType     wrapType = TextureWrapType::REPEAT;
    };

    class Texture {
    public:
        void Destroy();
        void Bind( i32 slot = 0 ) const;
        void Unbind( i32 slot = 0 ) const;

        bool IsValid() const { return handle != 0; }
        i32  GetWidth() const { return width; }
        i32  GetHeight() const { return height; }
        u32  GetHandle() const { return handle; }

        const LargeString & GetPath() const { return path; }

        void Serialize( Serializer & serializer, TextureCreateInfo info );

    private:
        void CreateGLObject( const std::vector<u8> & data, TextureCreateInfo info );

        u32 handle = 0;

        i32 width = 0;
        i32 height = 0;
        i32 channels = 0;
        LargeString path = {};
    };

    class Shader {
    public:
        bool CreateFromSource( const char * vertexSrc, const char * fragmentSrc );
        bool CreateFromFiles( const char * vertexPath, const char * fragmentPath );
        void Destroy();

        void Bind() const;
        void Unbind() const;

        bool IsValid() const { return programHandle != 0; }
        u32  GetHandle() const { return programHandle; }

        i32 GetUniformLocation( const char * name ) const;

        void SetInt( i32 location, i32 value ) const;
        void SetFloat( i32 location, f32 value ) const;
        void SetVec2( i32 location, const Vec2 & value ) const;
        void SetVec3( i32 location, const Vec3 & value ) const;
        void SetVec4( i32 location, const Vec4 & value ) const;
        void SetMat3( i32 location, const Mat3 & value ) const;
        void SetMat4( i32 location, const Mat4 & value ) const;
        void SetMat4Array( i32 location, const Mat4 * values, i32 count ) const;

        void SetInt( const char * name, i32 value ) const;
        void SetFloat( const char * name, f32 value ) const;
        void SetVec2( const char * name, const Vec2 & value ) const;
        void SetVec3( const char * name, const Vec3 & value ) const;
        void SetVec4( const char * name, const Vec4 & value ) const;
        void SetMat3( const char * name, const Mat3 & value ) const;
        void SetMat4( const char * name, const Mat4 & value ) const;
        void SetMat4Array( const char * name, const Mat4 * values, i32 count ) const;

    private:
        static u32 CompileStage( u32 type, const char * source );

        u32 programHandle = 0;
    };

    class Material {
    public:
        Vec3    albedo = Vec3( 1, 1, 1 );
        f32     metalic = 0;
        f32     roughness = 1;

        const Texture * albedoTexture = nullptr;
        const Texture * metalicTexture = nullptr;
        const Texture * roughnessTexture = nullptr;
    };
}
