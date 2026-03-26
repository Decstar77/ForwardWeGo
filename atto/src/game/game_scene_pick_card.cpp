
#include "game_scene_pick_card.h"

namespace atto {

    static f32 EaseOutCubic( f32 t ) {
        f32 inv = 1.0f - t;
        return 1.0f - inv * inv * inv;
    }

    static f32 EaseInCubic( f32 t ) {
        return t * t * t;
    }


    f32 GameScenePickCard::GetUIScale() const {
        Vec2i winSize = Engine::Get().GetWindowSize();
        return (f32)winSize.y / 720.0f;
    }

    void GameScenePickCard::OnStart( const char * args ) {
        nextMap = args ? args : "";

        Renderer & renderer = Engine::Get().GetRenderer();
        titleFont    = renderer.GetOrLoadFont( "assets/fonts/kenvector_future.ttf", 36.0f );
        cardNameFont = renderer.GetOrLoadFont( "assets/fonts/kenvector_future.ttf", 16.0f );
        cardDescFont = renderer.GetOrLoadFont( "assets/fonts/kenvector_future.ttf", 12.0f );
        backGroundTexture = renderer.GetOrLoadTexture( "assets/textures/ai-gen/background001.png", true );
        cardFrontTexture = renderer.GetOrLoadTexture( "assets/textures/cards/front.png", true );
        cardAttackSpeedIncreaseTexture = renderer.GetOrLoadTexture( "assets/textures/cards/attack-speed-increase.png", true );
        cardAttackDamageIncreaseTexture = renderer.GetOrLoadTexture( "assets/textures/cards/attack-damage-increase.png", true );
        cardAttackAccuracyIncreaseTexture = renderer.GetOrLoadTexture( "assets/textures/cards/attack-accuracy-increase.png", true );
        cardReloadSpeedIncreaseTexture = renderer.GetOrLoadTexture( "assets/textures/cards/reload-speed-increase.png", true );
        cardAmmoCapacityIncreaseTexture = renderer.GetOrLoadTexture( "assets/textures/cards/ammo-capacity-increase.png", true );
        cardMaxHealthIncreaseTexture = renderer.GetOrLoadTexture( "assets/textures/cards/max-health-increase.png", true );
        cardRestoreHealthTexture = renderer.GetOrLoadTexture( "assets/textures/cards/restore-health.png", true );
        cardExtraCoinsTexture = renderer.GetOrLoadTexture( "assets/textures/coin.png", true );
        cardEmptyGem =  renderer.GetOrLoadTexture( "assets/textures/cards/empty-gem.png", true );
        cardFullGem =  renderer.GetOrLoadTexture( "assets/textures/cards/full-gem.png", true );

        Engine::Get().GetInput().SetCursorCaptured( false );

        RollCards();

        phase      = PickCardPhase::SlideIn;
        phaseTimer = 0.0f;
        chosenCard = -1;
    }

    void GameScenePickCard::RollCards() {
        RNG & rng = Engine::Get().GetRNG();
        Vec2i winSize = Engine::Get().GetWindowSize();
        f32 scale   = GetUIScale();
        f32 cardW   = BaseCardWidth * scale;
        f32 cardH   = BaseCardHeight * scale;
        f32 cardGap = BaseCardGap * scale;
        f32 centerX = winSize.x * 0.5f;
        f32 centerY = winSize.y * 0.5f + 20.0f * scale;

        f32 totalWidth = NumCards * cardW + ( NumCards - 1 ) * cardGap;
        f32 startX = centerX - totalWidth * 0.5f + cardW * 0.5f;

        for ( i32 i = 0; i < NumCards; i++ ) {
            cards[i].type    = static_cast<PlayerCardType>( rng.Unsigned32( 0, PlayerCardTypeCountValue - 1 ) );
            cards[i].targetX = startX + i * ( cardW + cardGap );
            cards[i].targetY = centerY;
            cards[i].x       = cards[i].targetX;
            cards[i].y       = (f32)winSize.y + cardH;
            cards[i].hovered = false;
        }
    }

    bool GameScenePickCard::IsMouseOverCard( const PickCard & card, Vec2 mousePos ) const {
        f32 scale = GetUIScale();
        f32 halfW = BaseCardWidth * scale * 0.5f;
        f32 halfH = BaseCardHeight * scale * 0.5f;
        return mousePos.x >= card.x - halfW && mousePos.x <= card.x + halfW
            && mousePos.y >= card.y - halfH && mousePos.y <= card.y + halfH;
    }

    const Texture * GameScenePickCard::PlayerCardTypeToFeatureTexture( PlayerCardType type ) const {
        switch ( type ) {
            case PlayerCardType::AttackSpeedIncrease: return cardAttackSpeedIncreaseTexture;
            case PlayerCardType::AttackDamageIncrease: return cardAttackDamageIncreaseTexture;
            case PlayerCardType::AttackAccuracyIncrease: return cardAttackAccuracyIncreaseTexture;
            case PlayerCardType::AmmoCapacityIncrease:  return cardAmmoCapacityIncreaseTexture;
            case PlayerCardType::ReloadSpeedIncrease: return cardReloadSpeedIncreaseTexture;
            case PlayerCardType::MaxHealthIncrease: return cardMaxHealthIncreaseTexture;
            case PlayerCardType::RestoreHealth: return cardRestoreHealthTexture;
            case PlayerCardType::ExtraCoins_10: return cardExtraCoinsTexture;
            default: return nullptr;
        }
    }

    void GameScenePickCard::OnUpdate( f32 deltaTime ) {
        Input & input = Engine::Get().GetInput();
        Vec2i winSize = Engine::Get().GetWindowSize();
        f32 scale = GetUIScale();
        f32 cardH = BaseCardHeight * scale;

        phaseTimer += deltaTime;

        if ( phase == PickCardPhase::SlideIn ) {
            for ( i32 i = 0; i < NumCards; i++ ) {
                f32 cardDelay = i * 0.1f;
                f32 t = Saturate( ( phaseTimer - cardDelay ) / SlideInDuration );
                f32 eased = EaseOutCubic( t );
                f32 startY = (f32)winSize.y + cardH;
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

                f32 hoverLift = 15.0f * scale;
                f32 hoverTarget = cards[i].hovered ? cards[i].targetY - hoverLift : cards[i].targetY;
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
                    f32 exitY = -cardH * 1.5f;
                    cards[i].y = Lerp( cards[i].targetY, exitY, eased );
                }
                else {
                    f32 exitY = (f32)winSize.y + cardH * 1.5f;
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
        f32 scale = GetUIScale();

        i32 cardW = (i32)( BaseCardWidth * scale );
        i32 cardH = (i32)( BaseCardHeight * scale );

        renderer.SetClearColor( Color( 0.08f, 0.08f, 0.12f, 1.0f ) );
        renderer.SetViewport( 0, 0, vpW, vpH );

        ui.Begin( vpW, vpH );

        ui.DrawSprite( backGroundTexture, ui.GetCenterX(), ui.GetCenterY(), vpW, vpH );

        // Title
        ui.DrawText( titleFont, ui.GetCenterX(), 50.0f * scale, "Choose a Card",
                     Vec4( 1.0f ), UIAlignH::Center, UIAlignV::Top );

        for ( i32 i = 0; i < NumCards; i++ ) {
            const PickCard & card = cards[i];

            // Feature first
            const i32 featureYOffset = static_cast<i32>( card.y - 55 * scale );
            const i32 featureSize = static_cast<i32>( 140 * scale );
            const Texture * featureTexture = PlayerCardTypeToFeatureTexture( card.type );
            // Feature image
            ui.DrawSprite( featureTexture, card.x , featureYOffset, featureSize + 10 * scale, featureSize );

            // Card front frame
            ui.DrawSprite( cardFrontTexture, card.x, card.y, cardW, cardH );


            // Card name on the banner
            f32 bannerY = card.y + cardH * 0.2f;
            ui.DrawText( cardNameFont, card.x, bannerY,
                         PlayerCardTypeToName( card.type ),
                         Vec4( 0.0f, 0.0f, 0.0f, 1.0f ), UIAlignH::Center, UIAlignV::Center );

            // Card description in the lower panel
            f32 descY = card.y + cardH * 0.32f;
            ui.DrawText( cardDescFont, card.x, descY,
                         PlayerCardTypeToDescription( card.type ),
                         Vec4( 0.15f, 0.15f, 0.15f, 1.0f ), UIAlignH::Center, UIAlignV::Center );
        }

        ui.End( renderer );
    }

    void GameScenePickCard::OnShutdown() {
    }

    void GameScenePickCard::OnResize( i32 width, i32 height ) {
        f32 scale   = GetUIScale();
        f32 cardW   = BaseCardWidth * scale;
        f32 cardGap = BaseCardGap * scale;
        f32 centerX = width * 0.5f;
        f32 centerY = height * 0.5f + 20.0f * scale;
        f32 totalWidth = NumCards * cardW + ( NumCards - 1 ) * cardGap;
        f32 startX = centerX - totalWidth * 0.5f + cardW * 0.5f;

        for ( i32 i = 0; i < NumCards; i++ ) {
            cards[i].targetX = startX + i * ( cardW + cardGap );
            cards[i].targetY = centerY;
            cards[i].x = cards[i].targetX;
        }
    }
}
