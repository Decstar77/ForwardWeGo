#pragma once

#include "../atto_core.h"
#include "../atto_math.h"

namespace atto {

    struct Vertex {
        Vec3 position;
        Vec3 normal;
        Vec2 texCoords;
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
        i32 GetMeshCount() const { return static_cast<i32>( meshes.size() ); }

    private:
        std::vector<Mesh> meshes;
    };

} // namespace atto
