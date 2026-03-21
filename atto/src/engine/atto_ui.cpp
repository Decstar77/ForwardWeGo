#include "atto_ui.h"
#include "atto_renderer.h"

#include <cstring>

namespace atto {

    // =========================================================================
    // UIConstraint
    // =========================================================================

    UIConstraint UIConstraint::Create( Vec2 anchor, Vec2 pivot, Vec2 offset ) {
        UIConstraint c;
        c.anchor = anchor;
        c.pivot  = pivot;
        c.offset = offset;
        return c;
    }

    UIConstraint UIConstraint::FromPreset( UIAnchorPreset preset, Vec2 offset ) {
        UIConstraint c;
        c.offset = offset;
        switch ( preset ) {
            case UIAnchorPreset::TopLeft:      c.anchor = Vec2( 0.0f, 0.0f ); c.pivot = Vec2( 0.0f, 0.0f ); break;
            case UIAnchorPreset::TopCenter:    c.anchor = Vec2( 0.5f, 0.0f ); c.pivot = Vec2( 0.5f, 0.0f ); break;
            case UIAnchorPreset::TopRight:     c.anchor = Vec2( 1.0f, 0.0f ); c.pivot = Vec2( 1.0f, 0.0f ); break;
            case UIAnchorPreset::CenterLeft:   c.anchor = Vec2( 0.0f, 0.5f ); c.pivot = Vec2( 0.0f, 0.5f ); break;
            case UIAnchorPreset::Center:       c.anchor = Vec2( 0.5f, 0.5f ); c.pivot = Vec2( 0.5f, 0.5f ); break;
            case UIAnchorPreset::CenterRight:  c.anchor = Vec2( 1.0f, 0.5f ); c.pivot = Vec2( 1.0f, 0.5f ); break;
            case UIAnchorPreset::BottomLeft:   c.anchor = Vec2( 0.0f, 1.0f ); c.pivot = Vec2( 0.0f, 1.0f ); break;
            case UIAnchorPreset::BottomCenter: c.anchor = Vec2( 0.5f, 1.0f ); c.pivot = Vec2( 0.5f, 1.0f ); break;
            case UIAnchorPreset::BottomRight:  c.anchor = Vec2( 1.0f, 1.0f ); c.pivot = Vec2( 1.0f, 1.0f ); break;
        }
        return c;
    }

    // =========================================================================
    // Text measurement helpers
    // =========================================================================

    // Runs stbtt_GetBakedQuad with baseline at y=0 to compute the bounding box.
    // Returns the pixel size and the distance from the top of the bbox to the baseline.
    static void MeasureTextInternal( const Font * font, const char * text, Vec2 & outSize, f32 & outBaselineFromTop ) {
        outSize = Vec2( 0.0f );
        outBaselineFromTop = 0.0f;

        if ( !font || !font->IsValid() || !text || *text == '\0' ) {
            return;
        }

        const stbtt_bakedchar * charData = font->GetCharData();
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

            stbtt_aligned_quad q;
            stbtt_GetBakedQuad( charData, FONT_ATLAS_WIDTH, FONT_ATLAS_HEIGHT,
                                c - FONT_FIRST_CHAR, &xpos, &ypos, &q, 0 );

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
    }

    void UICanvas::Text( const UIConstraint & constraint, const Font * font, const char * text, Vec4 color ) {
        if ( !font || !text || *text == '\0' ) {
            return;
        }

        TextCmd & cmd = textCmds.AddEmpty();
        cmd.constraint = constraint;
        cmd.font       = font;
        cmd.color      = color;
        strncpy( cmd.text, text, sizeof( cmd.text ) - 1 );
        cmd.text[ sizeof( cmd.text ) - 1 ] = '\0';
    }

    void UICanvas::Sprite( const UIConstraint & constraint, const Texture * texture, i32 width, i32 height ) {
        if ( !texture ) {
            return;
        }

        SpriteCmd & cmd = spriteCmds.AddEmpty();
        cmd.constraint = constraint;
        cmd.texture    = texture;
        cmd.width      = width;
        cmd.height     = height;
    }

    void UICanvas::End( Renderer & renderer ) {
        const Vec2 viewport( (f32)vw, (f32)vh );

        // Resolve and render sprite commands
        const i32 spriteCount = spriteCmds.GetCount();
        for ( i32 i = 0; i < spriteCount; i++ ) {
            const SpriteCmd & cmd = spriteCmds[ i ];
            const Vec2 elemSize( (f32)cmd.width, (f32)cmd.height );

            // Constraint resolution: top-left pixel position
            Vec2 topLeft = cmd.constraint.anchor * viewport
                         + cmd.constraint.offset
                         - cmd.constraint.pivot * elemSize;

            // Pixel center → NDC  (screen Y down, NDC Y up)
            Vec2 pixelCenter = topLeft + elemSize * 0.5f;
            Vec2 ndcCenter;
            ndcCenter.x = ( pixelCenter.x / (f32)vw ) * 2.0f - 1.0f;
            ndcCenter.y = 1.0f - ( pixelCenter.y / (f32)vh ) * 2.0f;

            renderer.RenderSprite( cmd.texture, ndcCenter, cmd.width, cmd.height, vw, vh );
        }

        // Resolve and render text commands
        const i32 textCount = textCmds.GetCount();
        for ( i32 i = 0; i < textCount; i++ ) {
            const TextCmd & cmd = textCmds[ i ];

            Vec2 textSize;
            f32  baselineFromTop;
            MeasureTextInternal( cmd.font, cmd.text, textSize, baselineFromTop );

            Vec2 topLeft = cmd.constraint.anchor * viewport
                         + cmd.constraint.offset
                         - cmd.constraint.pivot * textSize;

            // DrawText expects the baseline y, not the top of the text box
            f32 baselineY = topLeft.y + baselineFromTop;

            renderer.DrawText( cmd.font, cmd.text, topLeft.x, baselineY, cmd.color, vw, vh );
        }
    }

} // namespace atto
