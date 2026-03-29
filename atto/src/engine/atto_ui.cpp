#include "atto_ui.h"
#include "atto_renderer.h"

#include <cstring>

namespace atto {

    // =========================================================================
    // Text measurement helpers
    // =========================================================================

    // Runs GetBakedQuad with baseline at y=0 to compute the bounding box.
    // Returns the pixel size and the distance from the top of the bbox to the baseline.
    static void MeasureTextInternal( const Font * font, const char * text, Vec2 & outSize, f32 & outBaselineFromTop ) {
        outSize = Vec2( 0.0f );
        outBaselineFromTop = 0.0f;

        if ( !font || !font->IsValid() || !text || *text == '\0' ) {
            return;
        }

        const BakedChar * charData = font->GetCharData();
        f32 xpos = 0.0f;
        f32 ypos = 0.0f;
        f32 minY =  1e9f;
        f32 maxY = -1e9f;
        f32 maxX = 0.0f;

        while ( *text ) {
            const char c = *text++;
            if ( c < FONT_FIRST_CHAR || c >= FONT_FIRST_CHAR + FONT_CHAR_COUNT ) {
                continue;
            }

            AlignedQuad q;
            GetBakedQuad( charData, FONT_ATLAS_WIDTH, FONT_ATLAS_HEIGHT,
                          c - FONT_FIRST_CHAR, &xpos, &ypos, &q, false );

            if ( q.y0 < minY ) minY = q.y0;
            if ( q.y1 > maxY ) maxY = q.y1;
            if ( q.x1 > maxX ) maxX = q.x1;
        }

        if ( maxX <= 0.0f ) {
            return;
        }

        outSize = Vec2( maxX, maxY - minY );
        outBaselineFromTop = -minY;    // minY is negative (above baseline)
    }

    Vec2 UICanvas::MeasureText( const Font * font, const char * text ) {
        Vec2 size;
        f32  baseline;
        MeasureTextInternal( font, text, size, baseline );
        return size;
    }

    // =========================================================================
    // UICanvas
    // =========================================================================

    void UICanvas::Begin( i32 viewportW, i32 viewportH ) {
        vw = viewportW;
        vh = viewportH;
        textCmds.Clear();
        spriteCmds.Clear();
        rectCmds.Clear();
    }

    void UICanvas::DrawText( const Font * font, f32 x, f32 y, const char * text,
                             Vec4 color, UIAlignH alignH, UIAlignV alignV ) {
        if ( !font || !text || *text == '\0' ) {
            return;
        }

        TextCmd & cmd = textCmds.AddEmpty();
        cmd.x      = x;
        cmd.y      = y;
        cmd.font   = font;
        cmd.color  = color;
        cmd.alignH = alignH;
        cmd.alignV = alignV;
        strncpy( cmd.text, text, sizeof( cmd.text ) - 1 );
        cmd.text[ sizeof( cmd.text ) - 1 ] = '\0';
    }

    void UICanvas::DrawSprite( const Texture * texture, f32 x, f32 y, i32 width, i32 height ) {
        if ( !texture ) {
            return;
        }

        SpriteCmd & cmd = spriteCmds.AddEmpty();
        cmd.x       = x;
        cmd.y       = y;
        cmd.texture = texture;
        cmd.width   = width;
        cmd.height  = height;
    }

    void UICanvas::DrawRect( f32 x, f32 y, i32 width, i32 height, const Vec4 & color ) {
        RectCmd & cmd = rectCmds.AddEmpty();
        cmd.x      = x;
        cmd.y      = y;
        cmd.width  = width;
        cmd.height = height;
        cmd.color  = color;
    }

    void UICanvas::End( Renderer & renderer ) {
        // Render rect commands (before sprites and text so they act as backgrounds)
        const i32 rectCount = rectCmds.GetCount();
        for ( i32 i = 0; i < rectCount; i++ ) {
            const RectCmd & cmd = rectCmds[ i ];
            Vec2 ndcCenter;
            ndcCenter.x = ( cmd.x / (f32)vw ) * 2.0f - 1.0f;
            ndcCenter.y = 1.0f - ( cmd.y / (f32)vh ) * 2.0f;
            renderer.RenderRect( ndcCenter, cmd.width, cmd.height, vw, vh, cmd.color );
        }

        // Render sprite commands
        const i32 spriteCount = spriteCmds.GetCount();
        for ( i32 i = 0; i < spriteCount; i++ ) {
            const SpriteCmd & cmd = spriteCmds[ i ];

            // Convert pixel center to NDC (screen Y down, NDC Y up)
            Vec2 ndcCenter;
            ndcCenter.x = ( cmd.x / (f32)vw ) * 2.0f - 1.0f;
            ndcCenter.y = 1.0f - ( cmd.y / (f32)vh ) * 2.0f;

            renderer.RenderSprite( cmd.texture, ndcCenter, cmd.width, cmd.height, vw, vh );
        }

        // Render text commands
        const i32 textCount = textCmds.GetCount();
        for ( i32 i = 0; i < textCount; i++ ) {
            const TextCmd & cmd = textCmds[ i ];

            Vec2 textSize;
            f32  baselineFromTop;
            MeasureTextInternal( cmd.font, cmd.text, textSize, baselineFromTop );

            // Resolve horizontal position to left edge
            f32 leftX = cmd.x;
            switch ( cmd.alignH ) {
                case UIAlignH::Left:   break;
                case UIAlignH::Center: leftX -= textSize.x * 0.5f; break;
                case UIAlignH::Right:  leftX -= textSize.x;        break;
            }

            // Resolve vertical position to top edge
            f32 topY = cmd.y;
            switch ( cmd.alignV ) {
                case UIAlignV::Top:    break;
                case UIAlignV::Center: topY -= textSize.y * 0.5f; break;
                case UIAlignV::Bottom: topY -= textSize.y;        break;
            }

            // DrawText expects the baseline y, not the top of the text box
            f32 baselineY = topY + baselineFromTop;

            renderer.DrawText( cmd.font, cmd.text, leftX, baselineY, cmd.color, vw, vh );
        }
    }

} // namespace atto
