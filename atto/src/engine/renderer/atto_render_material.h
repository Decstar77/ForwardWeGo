#pragma once

#include "../atto_core.h"
#include "../atto_math.h"
#include "../atto_assets.h"

namespace atto {
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

    class Shader {
    public:
        void Bind();
        void Unbind();

        void SetUniform( const char *  name, float v );
        // Add more here

    private:
        u32 programID;
    };

    class Material {
        std::string name;
        Texture albedo;

        void Bind();
    };

}
