#pragma once

/*
    Atto Engine - Immediate-Mode UI System
    Uses stb_truetype for font rendering.
    Simple screen-space drawing: specify pixel coordinates directly.
*/

#include "atto_core.h"
#include "atto_math.h"
#include "atto_containers.h"

#include "stb_truetype/stb_truetype.h"

namespace atto {

    constexpr i32 FONT_ATLAS_WIDTH  = 512;
    constexpr i32 FONT_ATLAS_HEIGHT = 512;
    constexpr i32 FONT_FIRST_CHAR   = 32;  // space
    constexpr i32 FONT_CHAR_COUNT   = 96;  // ASCII 32-127

    class Serializer;

    class Font {
    public:
        bool LoadFromFile( const char * filePath, f32 fontSize );
        void Serialize( Serializer & serializer );
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
    // Immediate-mode UI
    // =========================================================================

    // Horizontal text alignment relative to the x coordinate.
    enum class UIAlignH {
        Left,       // x is the left edge of the text
        Center,     // x is the horizontal center of the text
        Right,      // x is the right edge of the text
    };

    // Vertical text alignment relative to the y coordinate.
    enum class UIAlignV {
        Top,        // y is the top of the text
        Center,     // y is the vertical center of the text
        Bottom,     // y is the bottom of the text
    };

    // Forward-declared — Renderer lives in atto_renderer.h.
    class Renderer;
    class Texture;

    // Immediate-mode UI canvas.  Call Begin -> draw elements -> End each frame.
    // All coordinates are in screen-space pixels with (0,0) at the top-left.
    class UICanvas {
    public:
        void Begin( i32 viewportW, i32 viewportH );

        // Draw text at the given screen position.
        // alignH/alignV control how x,y relate to the text bounding box.
        void DrawText( const Font * font, f32 x, f32 y, const char * text,
                       Vec4 color = Vec4( 1.0f ),
                       UIAlignH alignH = UIAlignH::Left,
                       UIAlignV alignV = UIAlignV::Top );

        // Draw a sprite centered at the given screen position.
        void DrawSprite( const Texture * texture, f32 x, f32 y, i32 width, i32 height );

        // Draw a filled rectangle centered at the given screen position.
        void DrawRect( f32 x, f32 y, i32 width, i32 height, const Vec4 & color );

        // Flush all queued draw commands.
        void End( Renderer & renderer );

        // Utility — measure the pixel bounds of a string without drawing it.
        static Vec2 MeasureText( const Font * font, const char * text );

        // Viewport accessors for easy positioning.
        f32 GetWidth()   const { return (f32)vw; }
        f32 GetHeight()  const { return (f32)vh; }
        f32 GetCenterX() const { return (f32)vw * 0.5f; }
        f32 GetCenterY() const { return (f32)vh * 0.5f; }

    private:
        i32 vw = 0;
        i32 vh = 0;

        struct TextCmd {
            f32             x, y;
            const Font *    font;
            Vec4            color;
            UIAlignH        alignH;
            UIAlignV        alignV;
            char            text[ 256 ];
        };

        struct SpriteCmd {
            f32             x, y;
            const Texture * texture;
            i32             width;
            i32             height;
        };

        struct RectCmd {
            f32             x, y;
            i32             width;
            i32             height;
            Vec4            color;
        };

        FixedList<TextCmd, 128>   textCmds;
        FixedList<SpriteCmd, 128> spriteCmds;
        FixedList<RectCmd, 128>   rectCmds;
    };

} // namespace atto
