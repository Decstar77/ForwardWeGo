#pragma once

#include "engine/atto_engine.h"

namespace atto {

    // This is global state that persists between scene/map loads
    class GameGlobalState {
    public:
        static GameGlobalState& Get();

        i32     GetPlayerCoins() const;
        void    AddPlayerCoins( i32 amount );

    private:
        GameGlobalState();

        i32 playerCoins = 200;
    };
}

