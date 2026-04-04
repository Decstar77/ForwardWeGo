#pragma once

#include "game_player_card.h"
#include "engine/atto_engine.h"

namespace atto {

    // This is global state that persists between scene/map loads
    class GameGlobalState {
    public:
        static GameGlobalState& Get();

        i32     GetPlayerCoins() const;
        void    AddPlayerCoins( i32 amount );

        void    AddPlayerCard( PlayerCardType card );
        i32     GetCardCount( PlayerCardType card ) const;
        f32     GetAttackDamageMultiplier() const;
        f32     GetAccuracySpreadMultiplier() const;
        i32     GetBonusAmmoCapacity() const;
        i32     GetMaxHealth() const;
        bool    ConsumePendingFullHeal();

        // Settings
        f32     GetMouseSensitivity() const { return mouseSensitivity; }
        void    SetMouseSensitivity( f32 sens ) { mouseSensitivity = Clamp( sens, 0.01f, 1.0f ); }

        f32     GetGameplayVolume() const { return gameplayVolume; }
        void    SetGameplayVolume( f32 vol );

        f32     GetMusicVolume() const { return musicVolume; }
        void    SetMusicVolume( f32 vol );

    private:
        GameGlobalState();

        i32 playerCoins = 200;

        // Settings
        f32 mouseSensitivity = 0.1f;
        f32 gameplayVolume   = 1.0f;
        f32 musicVolume      = 1.0f;

        std::vector<PlayerCardType> playerCards;
        bool pendingFullHeal = false;
    };
}

