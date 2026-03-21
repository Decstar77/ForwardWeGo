#pragma once

/*
    Atto Engine - UI System
    Uses stb_truetype for font rendering.
    Constraint-based layout: each element is positioned by an anchor on
    the viewport, a pivot on the element, and a pixel offset.
*/

#include "atto_core.h"
#include "atto_math.h"
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

    // =========================================================================
    // Constraint-based UI
    // =========================================================================

    // Presets for common anchor/pivot combinations.
    enum class UIAnchorPreset {
        TopLeft,
        TopCenter,
        TopRight,
        CenterLeft,
        Center,
        CenterRight,
        BottomLeft,
        BottomCenter,
        BottomRight,
    };

    // Describes where an element sits on screen.
    //   anchor  — (0-1) point on the viewport  (0,0 = top-left, 1,1 = bottom-right)
    //   pivot   — (0-1) point on the element    (0,0 = top-left of element)
    //   offset  — pixel offset applied after anchor resolution
    struct UIConstraint {
        Vec2 anchor = Vec2( 0.0f );
        Vec2 pivot  = Vec2( 0.0f );
        Vec2 offset = Vec2( 0.0f );

        static UIConstraint Create( Vec2 anchor, Vec2 pivot, Vec2 offset = Vec2( 0.0f ) );
        static UIConstraint FromPreset( UIAnchorPreset preset, Vec2 offset = Vec2( 0.0f ) );
    };

    // Forward-declared — Renderer lives in atto_renderer.h.
    class Renderer;
    class Texture;

    // Immediate-mode UI canvas.  Call Begin → add elements → End each frame.
    class UICanvas {
    public:
        void Begin( i32 viewportW, i32 viewportH );

        void Text( const UIConstraint & constraint, const Font * font, const char * text, Vec4 color = Vec4( 1.0f ) );
        void Sprite( const UIConstraint & constraint, const Texture * texture, i32 width, i32 height );

        void End( Renderer & renderer );

        // Utility — measure the pixel bounds of a string without drawing it.
        static Vec2 MeasureText( const Font * font, const char * text );

    private:
        i32 vw = 0;
        i32 vh = 0;

        struct TextCmd {
            UIConstraint    constraint;
            const Font *    font;
            Vec4            color;
            char            text[ 256 ];
        };

        struct SpriteCmd {
            UIConstraint    constraint;
            const Texture * texture;
            i32             width;
            i32             height;
        };

        FixedList<TextCmd, 128>   textCmds;
        FixedList<SpriteCmd, 128> spriteCmds;
    };

} // namespace atto
