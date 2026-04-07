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

        i32     GetRoundCoins() const { return roundCoins; }
        void    AddRoundCoins( i32 amount );
        void    TransferRoundCoinsToMain();

        void    AddPlayerCard( PlayerCardType card );
        i32     GetCardCount( PlayerCardType card ) const;
        f32     GetAttackDamageMultiplier() const;
        f32     GetAccuracySpreadMultiplier() const;
        i32     GetBonusAmmoCapacity() const;
        i32     GetMaxHealth() const;
        f32     GetAttackSpeedMultiplier() const;
        f32     GetReloadSpeedMultiplier() const;

        i32     GetPlayerHealth() const { return playerHealth; }
        void    SetPlayerHealth( i32 hp ) { playerHealth = hp; }

        i32     GetCurrentRound() const { return currentRound; }
        void    IncrementRound() { currentRound++; }
        bool    IsGameComplete() const { return currentRound > MaxRounds; }
        static constexpr i32 MaxRounds = 10;

        void    ResetForNewGame();

        // Settings
        f32     GetMouseSensitivity() const { return mouseSensitivity; }
        void    SetMouseSensitivity( f32 sens ) { mouseSensitivity = Clamp( sens, 0.01f, 1.0f ); }

        f32     GetGameplayVolume() const { return gameplayVolume; }
        void    SetGameplayVolume( f32 vol );

        f32     GetMusicVolume() const { return musicVolume; }
        void    SetMusicVolume( f32 vol );

        const char * GetNextMap() const;

    private:
        GameGlobalState();

        i32 playerCoins = 0;
        i32 roundCoins = 0;
        i32 currentRound = 1;

        // Settings
        f32 mouseSensitivity = 0.1f;
        f32 gameplayVolume   = 1.0f;
        f32 musicVolume      = 1.0f;

        std::vector<PlayerCardType> playerCards;
        i32 playerHealth = 100;
    };
}

