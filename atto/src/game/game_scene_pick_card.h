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
        PlayerCard      playerCard;
        const Font *    titleFont    = nullptr;
        const Font *    cardNameFont = nullptr;
        const Font *    cardDescFont = nullptr;

        static constexpr i32 NumCards   = 3;
        static constexpr i32 CardWidth  = 200;
        static constexpr i32 CardHeight = 298; // Matches 1771:2633 ratio
        static constexpr i32 CardGap    = 60;
        static constexpr i32 GemSize    = 48;

        PickCard        cards[ NumCards ];
        PickCardPhase   phase        = PickCardPhase::SlideIn;
        f32             phaseTimer   = 0.0f;
        i32             chosenCard   = -1;

        static constexpr f32 SlideInDuration  = 0.6f;
        static constexpr f32 SlideOutDuration = 0.4f;
    };
}
