
#include "game_global_state.h"

namespace atto {
    GameGlobalState & GameGlobalState::Get() {
        static GameGlobalState instance;
        return instance;
    }

    void GameGlobalState::AddPlayerCoins( i32 amount ) {
        playerCoins += amount;
    }

    i32 GameGlobalState::GetPlayerCoins() const {
        return playerCoins;
    }

    void GameGlobalState::AddRoundCoins( i32 amount ) {
        roundCoins += amount;
    }

    void GameGlobalState::TransferRoundCoinsToMain() {
        playerCoins += roundCoins;
        roundCoins = 0;
    }

    void GameGlobalState::SetGameplayVolume( f32 vol ) {
        gameplayVolume = Clamp( vol, 0.0f, 1.0f );
        Engine::Get().GetAudioSystem().SetSFXVolume( gameplayVolume );
    }

    void GameGlobalState::SetMusicVolume( f32 vol ) {
        musicVolume = Clamp( vol, 0.0f, 1.0f );
        Engine::Get().GetAudioSystem().SetMusicVolume( musicVolume );
    }

    const char * GameGlobalState::GetNextMap() const {
        static const char * maps[] = {
            "assets/maps/game/level001.map",
            "assets/maps/game/level002.map",
            "assets/maps/game/level003.map",
            "assets/maps/game/level004.map",
            "assets/maps/game/level005.map",
        };

        const i32 index = Engine::Get().GetRNG().Signed32( 0, 4 );

        return maps[ index ];
    }

    void GameGlobalState::AddPlayerCard( PlayerCardType card ) {
        playerCards.push_back( card );
        if ( card == PlayerCardType::RestoreHealth ) {
            playerHealth = GetMaxHealth();
        }
    }

    i32 GameGlobalState::GetCardCount( PlayerCardType card ) const {
        i32 count = 0;
        for ( PlayerCardType c : playerCards ) {
            if ( c == card ) { count++; }
        }
        return count;
    }

    f32 GameGlobalState::GetAttackDamageMultiplier() const {
        // Each AttackDamageIncrease card gives +5% damage
        return 1.0f + 0.05f * (f32)GetCardCount( PlayerCardType::AttackDamageIncrease );
    }

    f32 GameGlobalState::GetAccuracySpreadMultiplier() const {
        // Each AttackAccuracyIncrease card reduces spread by 10%
        f32 mult = 1.0f - 0.10f * (f32)GetCardCount( PlayerCardType::AttackAccuracyIncrease );
        return Max( mult, 0.0f );
    }

    i32 GameGlobalState::GetBonusAmmoCapacity() const {
        // Each AmmoCapacityIncrease card gives +2 ammo
        return 2 * GetCardCount( PlayerCardType::AmmoCapacityIncrease );
    }

    i32 GameGlobalState::GetMaxHealth() const {
        // Base 100 + 25 per MaxHealthIncrease card
        return 100 + 25 * GetCardCount( PlayerCardType::MaxHealthIncrease );
    }

    f32 GameGlobalState::GetAttackSpeedMultiplier() const {
        // Each AttackSpeedIncrease card gives +5% attack speed
        return 1.0f + 0.05f * (f32)GetCardCount( PlayerCardType::AttackSpeedIncrease );
    }

    f32 GameGlobalState::GetReloadSpeedMultiplier() const {
        // Each ReloadSpeedIncrease card gives +5% reload speed
        return 1.0f + 0.05f * (f32)GetCardCount( PlayerCardType::ReloadSpeedIncrease );
    }

    void GameGlobalState::ResetForNewGame() {
        playerCards.clear();
        playerHealth = 100;
        roundCoins   = 0;
        currentRound = 1;
    }

    GameGlobalState::GameGlobalState() {
    }
}

