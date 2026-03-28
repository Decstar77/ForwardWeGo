#pragma once

#include "engine/atto_engine.h"
#include "game_player_card.h"

namespace atto {

    struct PickCard {
        PlayerCardType  type;
        f32             x;          // current screen x
        f32             targetX;    // resting screen x
        f32             y;          // current screen y
        f32             targetY;    // resting screen y
        bool            hovered;
        bool            soundPlayed;
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
        void            RollCards();
        bool            IsMouseOverCard( const PickCard & card, Vec2 mousePos ) const;
        const Texture * PlayerCardTypeToFeatureTexture( PlayerCardType type ) const;

        std::string     nextMap;
        UICanvas        ui;
        PlayerCard      playerCard;
        const Font *    titleFont    = nullptr;
        const Font *    cardNameFont = nullptr;
        const Font *    cardDescFont = nullptr;
        const Texture * backGroundTexture = nullptr;
        const Texture * cardFrontTexture = nullptr;
        const Texture * cardAttackSpeedIncreaseTexture = nullptr;
        const Texture * cardAttackDamageIncreaseTexture = nullptr;
        const Texture * cardAttackAccuracyIncreaseTexture = nullptr;
        const Texture * cardReloadSpeedIncreaseTexture = nullptr;
        const Texture * cardAmmoCapacityIncreaseTexture = nullptr;
        const Texture * cardMaxHealthIncreaseTexture = nullptr;
        const Texture * cardRestoreHealthTexture = nullptr;
        const Texture * cardExtraCoinsTexture = nullptr;
        const Texture * cardEmptyGem = nullptr;
        const Texture * cardFullGem = nullptr;

        SoundCollection sndCardSlide;

        f32 GetUIScale() const;

        static constexpr i32 NumCards       = 3;
        static constexpr i32 BaseCardWidth  = 200;
        static constexpr i32 BaseCardHeight = 298; // Matches 1771:2633 ratio
        static constexpr i32 BaseCardGap    = 60;
        static constexpr i32 BaseGemSize    = 48;
        static constexpr i32 CardFeatureWidth = 256;
        static constexpr i32 CardFeatureHeight = 256;

        PickCard        cards[ NumCards ];
        PickCardPhase   phase        = PickCardPhase::SlideIn;
        f32             phaseTimer   = 0.0f;
        i32             chosenCard   = -1;

        static constexpr f32 SlideInDuration  = 0.6f;
        static constexpr f32 SlideOutDuration = 0.4f;
    };
}
