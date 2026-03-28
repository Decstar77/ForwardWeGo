#include "game_scene_main_menu.h"
#include "game_global_state.h"

namespace atto {

    f32 GameSceneMainMenu::GetUIScale() const {
        Vec2i winSize = Engine::Get().GetWindowSize();
        return (f32)winSize.y / 720.0f;
    }

    bool GameSceneMainMenu::IsMouseOverRect( f32 cx, f32 cy, f32 w, f32 h, Vec2 mousePos ) const {
        f32 halfW = w * 0.5f;
        f32 halfH = h * 0.5f;
        return mousePos.x >= cx - halfW && mousePos.x <= cx + halfW
            && mousePos.y >= cy - halfH && mousePos.y <= cy + halfH;
    }

    bool GameSceneMainMenu::UpdateSlider( f32 cx, f32 cy, f32 trackWidth, f32 & value,
                                           Vec2 mousePos, bool mouseDown ) {
        f32 halfTrack = trackWidth * 0.5f;
        f32 leftX = cx - halfTrack;
        f32 rightX = cx + halfTrack;

        if ( !mouseDown ) {
            return false;
        }

        // Check if mouse is within the slider interaction area (generous height)
        f32 scale = GetUIScale();
        f32 interactH = 30.0f * scale;
        if ( mousePos.y < cy - interactH || mousePos.y > cy + interactH ) {
            return false;
        }

        f32 t = ( mousePos.x - leftX ) / ( rightX - leftX );
        t = Saturate( t );
        if ( t != value ) {
            value = t;
            return true;
        }
        return false;
    }

    void GameSceneMainMenu::DrawSlider( f32 cx, f32 cy, f32 trackWidth, f32 value,
                                         const char * label, const Font * font, bool hovered ) {
        f32 scale = GetUIScale();
        f32 trackH = BaseSliderTrackH * scale;
        f32 knobSize = BaseSliderKnobSize * scale;
        f32 halfTrack = trackWidth * 0.5f;

        // Label above the track
        ui.DrawText( font, cx, cy - 25.0f * scale, label,
                     Vec4( 1.0f ), UIAlignH::Center, UIAlignV::Center );

        // Track background
        Vec4 trackColor = Vec4( 0.3f, 0.3f, 0.35f, 0.9f );
        ui.DrawRect( cx, cy, (i32)trackWidth, (i32)trackH, trackColor );

        // Filled portion
        f32 filledW = trackWidth * value;
        if ( filledW > 1.0f ) {
            f32 filledCx = cx - halfTrack + filledW * 0.5f;
            Vec4 fillColor = hovered ? Vec4( 0.4f, 0.7f, 1.0f, 1.0f ) : Vec4( 0.3f, 0.6f, 0.9f, 1.0f );
            ui.DrawRect( filledCx, cy, (i32)filledW, (i32)trackH, fillColor );
        }

        // Knob
        f32 knobX = cx - halfTrack + trackWidth * value;
        Vec4 knobColor = hovered ? Vec4( 1.0f, 1.0f, 1.0f, 1.0f ) : Vec4( 0.85f, 0.85f, 0.9f, 1.0f );
        ui.DrawRect( knobX, cy, (i32)knobSize, (i32)knobSize, knobColor );

        // Value percentage to the right of the track
        char valText[16];
        snprintf( valText, sizeof( valText ), "%d%%", (i32)( value * 100.0f ) );
        ui.DrawText( font, cx + halfTrack + 40.0f * scale, cy, valText,
                     Vec4( 0.8f, 0.8f, 0.8f, 1.0f ), UIAlignH::Center, UIAlignV::Center );
    }

    // =========================================================================
    // Scene lifecycle
    // =========================================================================

    void GameSceneMainMenu::OnStart( const char * args ) {
        Renderer & renderer = Engine::Get().GetRenderer();

        titleFont       = renderer.GetOrLoadFont( "assets/fonts/kenvector_future.ttf", 48.0f );
        buttonFont      = renderer.GetOrLoadFont( "assets/fonts/kenvector_future.ttf", 24.0f );
        smallFont       = renderer.GetOrLoadFont( "assets/fonts/kenvector_future.ttf", 18.0f );
        coinFont        = renderer.GetOrLoadFont( "assets/fonts/kenvector_future.ttf", 22.0f );

        backgroundTexture = renderer.GetOrLoadTexture( "assets/textures/ai-gen/background001.png", true );
        coinTexture       = renderer.GetOrLoadTexture( "assets/textures/coin.png", true );

        sndButtonClick.Initialize();
        sndButtonClick.LoadSounds( { "assets/sounds/mainmenu/button-click.wav" } );

        sndButtonHover.Initialize();
        sndButtonHover.LoadSounds( { "assets/sounds/mainmenu/button-hover.wav" } );

        Engine::Get().GetInput().SetCursorCaptured( false );

        page = MainMenuPage::Main;
        activeSlider = -1;
        titleBob = 0.0f;
    }

    void GameSceneMainMenu::OnUpdate( f32 deltaTime ) {
        Input & input = Engine::Get().GetInput();
        Vec2 mousePos = input.GetMousePosition();
        bool mousePressed = input.IsMouseButtonPressed( MouseButton::Left );
        bool mouseDown = input.IsMouseButtonDown( MouseButton::Left );
        f32 scale = GetUIScale();
        Vec2i winSize = Engine::Get().GetWindowSize();

        titleBob += deltaTime;

        GameGlobalState & state = GameGlobalState::Get();

        if ( page == MainMenuPage::Main ) {
            // Play button — center screen
            f32 playW = BasePlayBtnW * scale;
            f32 playH = BasePlayBtnH * scale;
            f32 playCx = winSize.x * 0.5f;
            f32 playCy = winSize.y * 0.5f;

            bool wasPlayHovered = playHovered;
            playHovered = IsMouseOverRect( playCx, playCy, playW, playH, mousePos );
            if ( playHovered && !wasPlayHovered ) {
                sndButtonHover.Play( 0.3f );
            }
            if ( playHovered && mousePressed ) {
                sndButtonClick.Play( 0.5f );
                // TODO: Transition to map selection or first map
                Engine::Get().TransitionToScene( "Editor", "" );
            }

            // Bottom-left buttons
            f32 smallW = BaseSmallBtnW * scale;
            f32 smallH = BaseSmallBtnH * scale;
            f32 bottomY = (f32)winSize.y;
            f32 leftX = 0.0f;
            f32 padding = 15.0f * scale;

            // Weapons button
            f32 weaponsCx = leftX + padding + smallW * 0.5f;
            f32 weaponsCy = bottomY - padding - smallH * 0.5f;

            bool wasWeaponsHovered = weaponsHovered;
            weaponsHovered = IsMouseOverRect( weaponsCx, weaponsCy, smallW, smallH, mousePos );
            if ( weaponsHovered && !wasWeaponsHovered ) {
                sndButtonHover.Play( 0.3f );
            }
            if ( weaponsHovered && mousePressed ) {
                sndButtonClick.Play( 0.5f );
                page = MainMenuPage::Weapons;
            }

            // Upgrades button
            f32 upgradesCx = weaponsCx;
            f32 upgradesCy = weaponsCy - smallH - padding;

            bool wasUpgradesHovered = upgradesHovered;
            upgradesHovered = IsMouseOverRect( upgradesCx, upgradesCy, smallW, smallH, mousePos );
            if ( upgradesHovered && !wasUpgradesHovered ) {
                sndButtonHover.Play( 0.3f );
            }
            if ( upgradesHovered && mousePressed ) {
                sndButtonClick.Play( 0.5f );
                page = MainMenuPage::Upgrades;
            }

            // Options button — top left
            f32 optW = BaseSmallBtnW * scale * 0.7f;
            f32 optH = BaseSmallBtnH * scale * 0.7f;
            f32 optCx = padding + optW * 0.5f;
            f32 optCy = padding + optH * 0.5f;

            bool wasOptionsHovered = optionsHovered;
            optionsHovered = IsMouseOverRect( optCx, optCy, optW, optH, mousePos );
            if ( optionsHovered && !wasOptionsHovered ) {
                sndButtonHover.Play( 0.3f );
            }
            if ( optionsHovered && mousePressed ) {
                sndButtonClick.Play( 0.5f );
                page = MainMenuPage::Options;
            }
        }
        else if ( page == MainMenuPage::Options ) {
            f32 centerX = winSize.x * 0.5f;
            f32 startY = winSize.y * 0.35f;
            f32 sliderSpacing = 80.0f * scale;
            f32 trackW = BaseSliderTrackW * scale;

            // Mouse sensitivity slider
            f32 sensValue = ( state.GetMouseSensitivity() - 0.01f ) / ( 1.0f - 0.01f );
            f32 slider0Y = startY;
            bool sensChanged = UpdateSlider( centerX, slider0Y, trackW, sensValue, mousePos, mouseDown );
            if ( sensChanged ) {
                state.SetMouseSensitivity( Lerp( 0.01f, 1.0f, sensValue ) );
            }

            // Gameplay volume slider
            f32 gameVol = state.GetGameplayVolume();
            f32 slider1Y = startY + sliderSpacing;
            bool gameVolChanged = UpdateSlider( centerX, slider1Y, trackW, gameVol, mousePos, mouseDown );
            if ( gameVolChanged ) {
                state.SetGameplayVolume( gameVol );
            }

            // Music volume slider
            f32 musVol = state.GetMusicVolume();
            f32 slider2Y = startY + sliderSpacing * 2.0f;
            bool musVolChanged = UpdateSlider( centerX, slider2Y, trackW, musVol, mousePos, mouseDown );
            if ( musVolChanged ) {
                state.SetMusicVolume( musVol );
            }

            // Back button
            f32 smallW = BaseSmallBtnW * scale;
            f32 smallH = BaseSmallBtnH * scale;
            f32 backCx = winSize.x * 0.5f;
            f32 backCy = startY + sliderSpacing * 3.0f + 20.0f * scale;

            bool wasBackHovered = backHovered;
            backHovered = IsMouseOverRect( backCx, backCy, smallW, smallH, mousePos );
            if ( backHovered && !wasBackHovered ) {
                sndButtonHover.Play( 0.3f );
            }
            if ( backHovered && mousePressed ) {
                sndButtonClick.Play( 0.5f );
                page = MainMenuPage::Main;
            }

            // Escape to go back
            if ( input.IsKeyPressed( Key::Escape ) ) {
                page = MainMenuPage::Main;
            }
        }
        else if ( page == MainMenuPage::Weapons || page == MainMenuPage::Upgrades ) {
            // Back button — center bottom
            f32 smallW = BaseSmallBtnW * scale;
            f32 smallH = BaseSmallBtnH * scale;
            f32 backCx = winSize.x * 0.5f;
            f32 backCy = winSize.y * 0.75f;

            bool wasBackHovered = backHovered;
            backHovered = IsMouseOverRect( backCx, backCy, smallW, smallH, mousePos );
            if ( backHovered && !wasBackHovered ) {
                sndButtonHover.Play( 0.3f );
            }
            if ( backHovered && mousePressed ) {
                sndButtonClick.Play( 0.5f );
                page = MainMenuPage::Main;
            }

            if ( input.IsKeyPressed( Key::Escape ) ) {
                page = MainMenuPage::Main;
            }
        }
    }

    void GameSceneMainMenu::OnRender( Renderer & renderer ) {
        Vec2i winSize = Engine::Get().GetWindowSize();
        i32 vpW = winSize.x;
        i32 vpH = winSize.y;
        f32 scale = GetUIScale();
        Vec2 mousePos = Engine::Get().GetInput().GetMousePosition();

        renderer.SetClearColor( Color( 0.06f, 0.06f, 0.1f, 1.0f ) );
        renderer.SetViewport( 0, 0, vpW, vpH );

        ui.Begin( vpW, vpH );

        // Background
        //ui.DrawSprite( backgroundTexture, ui.GetCenterX(), ui.GetCenterY(), vpW, vpH );

        // Dim overlay for readability
        ui.DrawRect( ui.GetCenterX(), ui.GetCenterY(), vpW, vpH, Vec4( 0.0f, 0.0f, 0.0f, 0.3f ) );

        GameGlobalState & state = GameGlobalState::Get();

        if ( page == MainMenuPage::Main ) {
            // Title with gentle bob
            f32 titleY = vpH * 0.2f + sinf( titleBob * 1.5f ) * 5.0f * scale;
            ui.DrawText( titleFont, ui.GetCenterX(), titleY, "POLYGON SHOOTER",
                         Vec4( 1.0f, 0.9f, 0.3f, 1.0f ), UIAlignH::Center, UIAlignV::Center );

            // Subtitle
            ui.DrawText( smallFont, ui.GetCenterX(), titleY + 40.0f * scale, "An FPS Roguelike",
                         Vec4( 0.7f, 0.7f, 0.7f, 0.8f ), UIAlignH::Center, UIAlignV::Center );

            // Play button — center screen
            f32 playW = BasePlayBtnW * scale;
            f32 playH = BasePlayBtnH * scale;
            f32 playCx = ui.GetCenterX();
            f32 playCy = ui.GetCenterY();

            Vec4 playBgColor = playHovered ? Vec4( 0.25f, 0.55f, 0.9f, 0.95f ) : Vec4( 0.15f, 0.4f, 0.75f, 0.9f );
            ui.DrawRect( playCx, playCy, (i32)playW, (i32)playH, playBgColor );
            // Button border highlight
            ui.DrawRect( playCx, playCy - playH * 0.5f + 2.0f * scale, (i32)playW, (i32)( 4.0f * scale ),
                         Vec4( 0.4f, 0.7f, 1.0f, playHovered ? 1.0f : 0.5f ) );
            ui.DrawText( buttonFont, playCx, playCy, "PLAY",
                         Vec4( 1.0f ), UIAlignH::Center, UIAlignV::Center );

            // Bottom-left buttons
            f32 smallW = BaseSmallBtnW * scale;
            f32 smallH = BaseSmallBtnH * scale;
            f32 padding = 15.0f * scale;

            // Weapons button
            f32 weaponsCx = padding + smallW * 0.5f;
            f32 weaponsCy = (f32)vpH - padding - smallH * 0.5f;

            Vec4 weaponsBg = weaponsHovered ? Vec4( 0.3f, 0.5f, 0.3f, 0.9f ) : Vec4( 0.2f, 0.35f, 0.2f, 0.8f );
            ui.DrawRect( weaponsCx, weaponsCy, (i32)smallW, (i32)smallH, weaponsBg );
            ui.DrawText( smallFont, weaponsCx, weaponsCy, "Weapons",
                         Vec4( 1.0f ), UIAlignH::Center, UIAlignV::Center );

            // Upgrades button
            f32 upgradesCx = weaponsCx;
            f32 upgradesCy = weaponsCy - smallH - padding;

            Vec4 upgradesBg = upgradesHovered ? Vec4( 0.5f, 0.35f, 0.2f, 0.9f ) : Vec4( 0.35f, 0.25f, 0.15f, 0.8f );
            ui.DrawRect( upgradesCx, upgradesCy, (i32)smallW, (i32)smallH, upgradesBg );
            ui.DrawText( smallFont, upgradesCx, upgradesCy, "Upgrades",
                         Vec4( 1.0f ), UIAlignH::Center, UIAlignV::Center );

            // Player coins — top right
            {
                char coinText[32];
                snprintf( coinText, sizeof( coinText ), "%d", state.GetPlayerCoins() );
                Vec2 textSize = UICanvas::MeasureText( coinFont, coinText );
                f32 textX = ui.GetWidth() - 20.0f * scale;
                f32 topY = 20.0f * scale;
                f32 iconX = textX - textSize.x - 20.0f * scale;
                i32 iconSize = (i32)( 28.0f * scale );
                ui.DrawSprite( coinTexture, iconX, topY + textSize.y * 0.5f, iconSize, iconSize );
                ui.DrawText( coinFont, textX, topY, coinText, Vec4( 1.0f, 0.85f, 0.0f, 1.0f ),
                             UIAlignH::Right, UIAlignV::Top );
            }

            // Options button — top left
            f32 optW = BaseSmallBtnW * scale * 0.7f;
            f32 optH = BaseSmallBtnH * scale * 0.7f;
            f32 optCx = padding + optW * 0.5f;
            f32 optCy = padding + optH * 0.5f;

            Vec4 optBg = optionsHovered ? Vec4( 0.4f, 0.4f, 0.45f, 0.9f ) : Vec4( 0.25f, 0.25f, 0.3f, 0.8f );
            ui.DrawRect( optCx, optCy, (i32)optW, (i32)optH, optBg );
            ui.DrawText( smallFont, optCx, optCy, "Options",
                         Vec4( 0.9f, 0.9f, 0.9f, 1.0f ), UIAlignH::Center, UIAlignV::Center );
        }
        else if ( page == MainMenuPage::Options ) {
            // Title
            ui.DrawText( titleFont, ui.GetCenterX(), vpH * 0.15f, "OPTIONS",
                         Vec4( 1.0f, 0.9f, 0.3f, 1.0f ), UIAlignH::Center, UIAlignV::Center );

            f32 centerX = ui.GetCenterX();
            f32 startY = vpH * 0.35f;
            f32 sliderSpacing = 80.0f * scale;
            f32 trackW = BaseSliderTrackW * scale;

            // Mouse sensitivity
            f32 sensValue = ( state.GetMouseSensitivity() - 0.01f ) / ( 1.0f - 0.01f );
            f32 slider0Y = startY;
            bool sensHover = IsMouseOverRect( centerX, slider0Y, trackW + 60.0f * scale, 50.0f * scale, mousePos );
            DrawSlider( centerX, slider0Y, trackW, sensValue, "Mouse Sensitivity", smallFont, sensHover );

            // Gameplay volume
            f32 gameVol = state.GetGameplayVolume();
            f32 slider1Y = startY + sliderSpacing;
            bool gameVolHover = IsMouseOverRect( centerX, slider1Y, trackW + 60.0f * scale, 50.0f * scale, mousePos );
            DrawSlider( centerX, slider1Y, trackW, gameVol, "Gameplay Volume", smallFont, gameVolHover );

            // Music volume
            f32 musVol = state.GetMusicVolume();
            f32 slider2Y = startY + sliderSpacing * 2.0f;
            bool musVolHover = IsMouseOverRect( centerX, slider2Y, trackW + 60.0f * scale, 50.0f * scale, mousePos );
            DrawSlider( centerX, slider2Y, trackW, musVol, "Music Volume", smallFont, musVolHover );

            // Back button
            f32 smallW = BaseSmallBtnW * scale;
            f32 smallH = BaseSmallBtnH * scale;
            f32 backCx = ui.GetCenterX();
            f32 backCy = startY + sliderSpacing * 3.0f + 20.0f * scale;

            Vec4 backBg = backHovered ? Vec4( 0.5f, 0.3f, 0.3f, 0.9f ) : Vec4( 0.35f, 0.2f, 0.2f, 0.8f );
            ui.DrawRect( backCx, backCy, (i32)smallW, (i32)smallH, backBg );
            ui.DrawText( smallFont, backCx, backCy, "Back",
                         Vec4( 1.0f ), UIAlignH::Center, UIAlignV::Center );
        }
        else if ( page == MainMenuPage::Weapons ) {
            ui.DrawText( titleFont, ui.GetCenterX(), vpH * 0.15f, "WEAPONS",
                         Vec4( 1.0f, 0.9f, 0.3f, 1.0f ), UIAlignH::Center, UIAlignV::Center );

            ui.DrawText( smallFont, ui.GetCenterX(), ui.GetCenterY(), "Coming soon...",
                         Vec4( 0.6f, 0.6f, 0.6f, 0.8f ), UIAlignH::Center, UIAlignV::Center );

            // Back button
            f32 smallW = BaseSmallBtnW * scale;
            f32 smallH = BaseSmallBtnH * scale;
            f32 backCx = ui.GetCenterX();
            f32 backCy = vpH * 0.75f;

            Vec4 backBg = backHovered ? Vec4( 0.5f, 0.3f, 0.3f, 0.9f ) : Vec4( 0.35f, 0.2f, 0.2f, 0.8f );
            ui.DrawRect( backCx, backCy, (i32)smallW, (i32)smallH, backBg );
            ui.DrawText( smallFont, backCx, backCy, "Back",
                         Vec4( 1.0f ), UIAlignH::Center, UIAlignV::Center );
        }
        else if ( page == MainMenuPage::Upgrades ) {
            ui.DrawText( titleFont, ui.GetCenterX(), vpH * 0.15f, "UPGRADES",
                         Vec4( 1.0f, 0.9f, 0.3f, 1.0f ), UIAlignH::Center, UIAlignV::Center );

            ui.DrawText( smallFont, ui.GetCenterX(), ui.GetCenterY(), "Coming soon...",
                         Vec4( 0.6f, 0.6f, 0.6f, 0.8f ), UIAlignH::Center, UIAlignV::Center );

            // Back button
            f32 smallW = BaseSmallBtnW * scale;
            f32 smallH = BaseSmallBtnH * scale;
            f32 backCx = ui.GetCenterX();
            f32 backCy = vpH * 0.75f;

            Vec4 backBg = backHovered ? Vec4( 0.5f, 0.3f, 0.3f, 0.9f ) : Vec4( 0.35f, 0.2f, 0.2f, 0.8f );
            ui.DrawRect( backCx, backCy, (i32)smallW, (i32)smallH, backBg );
            ui.DrawText( smallFont, backCx, backCy, "Back",
                         Vec4( 1.0f ), UIAlignH::Center, UIAlignV::Center );
        }

        ui.End( renderer );
    }

    void GameSceneMainMenu::OnShutdown() {
    }

    void GameSceneMainMenu::OnResize( i32 width, i32 height ) {
    }
}
