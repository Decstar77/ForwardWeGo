#pragma once

#include "../atto_core.h"
#include "../atto_math.h"
#include "../atto_assets.h"

namespace atto {

    struct Vertex {
        Vec3 position;
        Vec3 normal;
        Vec2 texCoords;
    };

    class Texture {
    public:
        void LoadFromFile( const char * filePath );
        void Destroy();

        i32 GetWidth() const { return width; }
        i32 GetHeight() const { return height; }

    private:
        i32 width = 0;
        i32 height = 0;
        u32 texture = 0;
    };

    // Will add more later
    struct Material {
        std::string name;
        Texture albedo;
    };

    class Mesh {
    public:
        void Create( const std::vector<Vertex> & vertices, const std::vector<u32> & indices );
        void Destroy();
        void Draw() const;

        i32 GetIndexCount() const { return indexCount; }

    private:
        u32 vao = 0;
        u32 vbo = 0;
        u32 ebo = 0;
        i32 indexCount = 0;
    };

    class StaticModel {
    public:
        void LoadFromFile( const char * filePath, f32 scale = 1.0f );
        void Destroy();
        void Draw() const;

        bool IsLoaded() const { return !meshes.empty(); }
        i32 GetMeshCount() const { return static_cast<i32>(meshes.size()); }

    private:
        std::vector<Mesh> meshes;
    };

    class Brush {
    public:
        void ToStaticModel( StaticModel & model ) const;
        void Serialize( Serializer & serializer );

    private:
        struct VertexPlane {
            Vec3 v1;
            Vec3 v2;
            Vec3 v3;
            Vec3 v4;
            Vec3 normal;

            void Serialize( Serializer & serializer );
        };

        std::vector<VertexPlane> planes;
    };

} // namespace atto
