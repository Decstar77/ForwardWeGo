#pragma once

/*
    Atto Engine - UI System
    Uses stb_truetype for font rendering
*/

#include "atto_core.h"
#include "atto_containers.h"

#include "stb_truetype/stb_truetype.h"

namespace atto {

    constexpr i32 FONT_ATLAS_WIDTH  = 512;
    constexpr i32 FONT_ATLAS_HEIGHT = 512;
    constexpr i32 FONT_FIRST_CHAR   = 32;  // space
    constexpr i32 FONT_CHAR_COUNT   = 96;  // ASCII 32–127

    class Font {
    public:
        bool LoadFromFile( const char * filePath, f32 fontSize );
        bool IsValid() const { return atlasHandle != 0; }

        const LargeString & GetPath()     const { return path; }
        f32                 GetFontSize() const { return fontSize; }
        u32                 GetAtlasHandle() const { return atlasHandle; }
        i32                 GetAtlasWidth()  const { return FONT_ATLAS_WIDTH; }
        i32                 GetAtlasHeight() const { return FONT_ATLAS_HEIGHT; }

        const stbtt_bakedchar * GetCharData() const { return charData; }

    private:
        LargeString       path;
        f32               fontSize    = 0.0f;
        u32               atlasHandle = 0;
        stbtt_bakedchar   charData[ FONT_CHAR_COUNT ];
    };

} // namespace atto
