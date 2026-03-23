
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

    GameGlobalState::GameGlobalState() {
    }
}

