#pragma once

#include "engine/atto_engine.h"

namespace atto {

    enum class CardType {
        MaxHealth,
        MoveSpeed,
        ReloadSpeed,
        ExtraAmmo,
        DamageBoost,

        CardTypeCount
    };

    static constexpr i32 CardTypeCountValue = static_cast<i32>( CardType::CardTypeCount );

    inline const char * CardTypeToName( CardType type ) {
        switch ( type ) {
            case CardType::MaxHealth:    return "Vitality";
            case CardType::MoveSpeed:    return "Swift Boots";
            case CardType::ReloadSpeed:  return "Quick Hands";
            case CardType::ExtraAmmo:    return "Deep Pockets";
            case CardType::DamageBoost:  return "Sharp Rounds";
            default:                     return "???";
        }
    }

    inline const char * CardTypeToDescription( CardType type ) {
        switch ( type ) {
            case CardType::MaxHealth:    return "+25 Max Health";
            case CardType::MoveSpeed:    return "+15% Move Speed";
            case CardType::ReloadSpeed:  return "+20% Reload Speed";
            case CardType::ExtraAmmo:    return "+4 Magazine Size";
            case CardType::DamageBoost:  return "+10% Damage";
            default:                     return "";
        }
    }

    inline Vec4 CardTypeToColor( CardType type ) {
        switch ( type ) {
            case CardType::MaxHealth:    return Vec4( 0.8f, 0.2f, 0.2f, 1.0f );
            case CardType::MoveSpeed:    return Vec4( 0.2f, 0.7f, 0.3f, 1.0f );
            case CardType::ReloadSpeed:  return Vec4( 0.9f, 0.7f, 0.1f, 1.0f );
            case CardType::ExtraAmmo:    return Vec4( 0.2f, 0.4f, 0.8f, 1.0f );
            case CardType::DamageBoost:  return Vec4( 0.8f, 0.4f, 0.1f, 1.0f );
            default:                     return Vec4( 0.5f, 0.5f, 0.5f, 1.0f );
        }
    }

    struct PickCard {
        CardType    type;
        f32         x;          // current screen x position
        f32         targetX;    // target x position (at rest)
        f32         y;          // current screen y position
        f32         targetY;    // target y position (at rest)
        bool        hovered;
    };

    enum class PickCardPhase {
        SlideIn,
        Choosing,
        SlideOut,
    };

    class GameScenePickCard : public SceneInterface {
    public:
        static const char * GetSceneNameStatic() { return "GameScenePickCard"; }
        const char * GetSceneName() override { return GetSceneNameStatic(); }

        void OnStart( const char * args ) override;
        void OnUpdate( f32 deltaTime ) override;
        void OnRender( Renderer & renderer ) override;
        void OnShutdown() override;
        void OnResize( i32 width, i32 height ) override;

    private:
        void RollCards();
        bool IsMouseOverCard( const PickCard & card, Vec2 mousePos ) const;

        std::string     nextMap;
        UICanvas        ui;
        const Font *    titleFont    = nullptr;
        const Font *    cardFont     = nullptr;
        const Font *    descFont     = nullptr;

        static constexpr i32 NumCards   = 3;
        static constexpr i32 CardWidth  = 200;
        static constexpr i32 CardHeight = 280;
        static constexpr i32 CardGap    = 40;

        PickCard        cards[ NumCards ];
        PickCardPhase   phase        = PickCardPhase::SlideIn;
        f32             phaseTimer   = 0.0f;
        i32             chosenCard   = -1;

        static constexpr f32 SlideInDuration  = 0.6f;
        static constexpr f32 SlideOutDuration = 0.4f;
    };
}
