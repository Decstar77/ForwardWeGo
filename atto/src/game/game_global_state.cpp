
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

    void GameGlobalState::SetGameplayVolume( f32 vol ) {
        gameplayVolume = Clamp( vol, 0.0f, 1.0f );
        Engine::Get().GetAudioSystem().SetSFXVolume( gameplayVolume );
    }

    void GameGlobalState::SetMusicVolume( f32 vol ) {
        musicVolume = Clamp( vol, 0.0f, 1.0f );
        Engine::Get().GetAudioSystem().SetMusicVolume( musicVolume );
    }

    GameGlobalState::GameGlobalState() {
    }
}

