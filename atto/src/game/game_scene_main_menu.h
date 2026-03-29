#pragma once

#include "engine/atto_engine.h"

namespace atto {

    enum class MainMenuPage {
        Main,
        Options,
        Weapons,
        Upgrades,
    };

    class GameSceneMainMenu : public SceneInterface {
    public:
        static const char * GetSceneNameStatic() { return "GameSceneMainMenu"; }
        const char * GetSceneName() override { return GetSceneNameStatic(); }

        void OnStart( const char * args ) override;
        void OnUpdate( f32 deltaTime ) override;
        void OnRender( Renderer & renderer ) override;
        void OnShutdown() override;
        void OnResize( i32 width, i32 height ) override;

    private:
        f32 GetUIScale() const;

        // Hit testing helpers
        bool IsMouseOverRect( f32 cx, f32 cy, f32 w, f32 h, Vec2 mousePos ) const;

        // Slider: returns true if value changed. cx/cy is center of the slider track.
        bool UpdateSlider( f32 cx, f32 cy, f32 trackWidth, f32 & value, Vec2 mousePos, bool mouseDown );
        void DrawSlider( f32 cx, f32 cy, f32 trackWidth, f32 value, const char * label,
                         const Font * font, bool hovered );

        UICanvas        ui;
        MainMenuPage    page = MainMenuPage::Main;

        std::string     args;

        // Fonts
        const Font *    titleFont       = nullptr;
        const Font *    buttonFont      = nullptr;
        const Font *    smallFont       = nullptr;
        const Font *    coinFont        = nullptr;

        // Textures
        const Texture * backgroundTexture = nullptr;
        const Texture * coinTexture       = nullptr;

        // Sounds
        SoundCollection sndButtonClick;
        SoundCollection sndButtonHover;

        // Button hover states (for hover sound deduplication)
        bool playHovered       = false;
        bool weaponsHovered    = false;
        bool upgradesHovered   = false;
        bool optionsHovered    = false;
        bool backHovered       = false;

        // Slider drag state
        i32  activeSlider      = -1;  // Which slider is being dragged (-1 = none)

        // Animation
        f32  titleBob          = 0.0f;

        // Button constants
        static constexpr f32 BasePlayBtnW      = 280.0f;
        static constexpr f32 BasePlayBtnH       = 70.0f;
        static constexpr f32 BaseSmallBtnW       = 200.0f;
        static constexpr f32 BaseSmallBtnH       = 50.0f;
        static constexpr f32 BaseSliderTrackW    = 300.0f;
        static constexpr f32 BaseSliderTrackH    = 8.0f;
        static constexpr f32 BaseSliderKnobSize  = 24.0f;
    };

}
