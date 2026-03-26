
#include "game_scene_pick_card.h"

namespace atto {

    static f32 EaseOutCubic( f32 t ) {
        f32 inv = 1.0f - t;
        return 1.0f - inv * inv * inv;
    }

    static f32 EaseInCubic( f32 t ) {
        return t * t * t;
    }

    void GameScenePickCard::OnStart( const char * args ) {
        nextMap = args ? args : "";

        Renderer & renderer = Engine::Get().GetRenderer();
        titleFont    = renderer.GetOrLoadFont( "assets/fonts/kenvector_future.ttf", 36.0f );
        cardNameFont = renderer.GetOrLoadFont( "assets/fonts/kenvector_future.ttf", 16.0f );
        cardDescFont = renderer.GetOrLoadFont( "assets/fonts/kenvector_future.ttf", 12.0f );

        playerCard.Initialize();

        Engine::Get().GetInput().SetCursorCaptured( false );

        RollCards();

        phase      = PickCardPhase::SlideIn;
        phaseTimer = 0.0f;
        chosenCard = -1;
    }

    void GameScenePickCard::RollCards() {
        RNG & rng = Engine::Get().GetRNG();
        Vec2i winSize = Engine::Get().GetWindowSize();
        f32 centerX = winSize.x * 0.5f;
        f32 centerY = winSize.y * 0.5f + 20.0f; // Slight offset for title

        f32 totalWidth = (f32)( NumCards * CardWidth + ( NumCards - 1 ) * CardGap );
        f32 startX = centerX - totalWidth * 0.5f + CardWidth * 0.5f;

        for ( i32 i = 0; i < NumCards; i++ ) {
            cards[i].type    = static_cast<PlayerCardType>( rng.Unsigned32( 0, PlayerCardTypeCountValue - 1 ) );
            cards[i].targetX = startX + i * ( CardWidth + CardGap );
            cards[i].targetY = centerY;
            cards[i].x       = cards[i].targetX;
            cards[i].y       = (f32)winSize.y + CardHeight;
            cards[i].hovered = false;
        }
    }

    bool GameScenePickCard::IsMouseOverCard( const PickCard & card, Vec2 mousePos ) const {
        f32 halfW = CardWidth * 0.5f;
        f32 halfH = CardHeight * 0.5f;
        return mousePos.x >= card.x - halfW && mousePos.x <= card.x + halfW
            && mousePos.y >= card.y - halfH && mousePos.y <= card.y + halfH;
    }

    void GameScenePickCard::OnUpdate( f32 deltaTime ) {
        Input & input = Engine::Get().GetInput();
        Vec2i winSize = Engine::Get().GetWindowSize();

        phaseTimer += deltaTime;

        if ( phase == PickCardPhase::SlideIn ) {
            for ( i32 i = 0; i < NumCards; i++ ) {
                f32 cardDelay = i * 0.1f;
                f32 t = Saturate( ( phaseTimer - cardDelay ) / SlideInDuration );
                f32 eased = EaseOutCubic( t );
                f32 startY = (f32)winSize.y + CardHeight;
                cards[i].y = Lerp( startY, cards[i].targetY, eased );
            }

            f32 lastCardFinish = ( NumCards - 1 ) * 0.1f + SlideInDuration;
            if ( phaseTimer >= lastCardFinish ) {
                phase = PickCardPhase::Choosing;
                phaseTimer = 0.0f;
                for ( i32 i = 0; i < NumCards; i++ ) {
                    cards[i].y = cards[i].targetY;
                }
            }
        }
        else if ( phase == PickCardPhase::Choosing ) {
            Vec2 mousePos = input.GetMousePosition();

            for ( i32 i = 0; i < NumCards; i++ ) {
                cards[i].hovered = IsMouseOverCard( cards[i], mousePos );

                f32 hoverTarget = cards[i].hovered ? cards[i].targetY - 15.0f : cards[i].targetY;
                cards[i].y += ( hoverTarget - cards[i].y ) * Min( 12.0f * deltaTime, 1.0f );
            }

            if ( input.IsMouseButtonPressed( MouseButton::Left ) ) {
                for ( i32 i = 0; i < NumCards; i++ ) {
                    if ( cards[i].hovered ) {
                        chosenCard = i;
                        phase = PickCardPhase::SlideOut;
                        phaseTimer = 0.0f;
                        break;
                    }
                }
            }
        }
        else if ( phase == PickCardPhase::SlideOut ) {
            for ( i32 i = 0; i < NumCards; i++ ) {
                f32 t = Saturate( phaseTimer / SlideOutDuration );
                f32 eased = EaseInCubic( t );

                if ( i == chosenCard ) {
                    f32 exitY = -CardHeight * 1.5f;
                    cards[i].y = Lerp( cards[i].targetY, exitY, eased );
                }
                else {
                    f32 exitY = (f32)winSize.y + CardHeight * 1.5f;
                    cards[i].y = Lerp( cards[i].targetY, exitY, eased );
                }
            }

            if ( phaseTimer >= SlideOutDuration + 0.15f ) {
                // TODO: Apply card effect to global state
                if ( !nextMap.empty() ) {
                    Engine::Get().TransitionToScene( "GameMapScene", nextMap.c_str() );
                }
                else {
                    Engine::Get().TransitionToScene( "Editor", "" );
                }
            }
        }
    }

    void GameScenePickCard::OnRender( Renderer & renderer ) {
        Vec2i winSize = Engine::Get().GetWindowSize();
        i32 vpW = winSize.x;
        i32 vpH = winSize.y;

        renderer.SetClearColor( Color( 0.08f, 0.08f, 0.12f, 1.0f ) );
        renderer.SetViewport( 0, 0, vpW, vpH );

        ui.Begin( vpW, vpH );

        // Title
        ui.DrawText( titleFont, ui.GetCenterX(), 50.0f, "Choose a Card",
                     Vec4( 1.0f ), UIAlignH::Center, UIAlignV::Top );

        for ( i32 i = 0; i < NumCards; i++ ) {
            const PickCard & card = cards[i];

            // Card front frame on top
            ui.DrawSprite( playerCard.GetFront(), card.x, card.y, CardWidth, CardHeight );

            // Full gem centered in the upper arch area of the front texture
            // The arch window is roughly the top 55% of the card
            //f32 gemY = card.y - CardHeight * 0.15f;
            //ui.DrawSprite( playerCard.GetFullGem(), card.x, gemY, GemSize, GemSize );

            // Card name on the banner (roughly 55% down from card top)
            f32 bannerY = card.y + CardHeight * 0.2f;
            ui.DrawText( cardNameFont, card.x, bannerY,
                         PlayerCardTypeToName( card.type ),
                         Vec4( 0.0f, 0.0f, 0.0f, 1.0f ), UIAlignH::Center, UIAlignV::Center );

            // Card description in the lower panel (roughly 75% down)
            f32 descY = card.y + CardHeight * 0.32f;
            ui.DrawText( cardDescFont, card.x, descY,
                         PlayerCardTypeToDescription( card.type ),
                         Vec4( 0.15f, 0.15f, 0.15f, 1.0f ), UIAlignH::Center, UIAlignV::Center );
        }

        ui.End( renderer );
    }

    void GameScenePickCard::OnShutdown() {
    }

    void GameScenePickCard::OnResize( i32 width, i32 height ) {
        f32 centerX = width * 0.5f;
        f32 centerY = height * 0.5f + 20.0f;
        f32 totalWidth = (f32)( NumCards * CardWidth + ( NumCards - 1 ) * CardGap );
        f32 startX = centerX - totalWidth * 0.5f + CardWidth * 0.5f;

        for ( i32 i = 0; i < NumCards; i++ ) {
            cards[i].targetX = startX + i * ( CardWidth + CardGap );
            cards[i].targetY = centerY;
            cards[i].x = cards[i].targetX;
        }
    }
}
