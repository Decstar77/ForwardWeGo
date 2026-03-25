#pragma once

#include "engine/atto_engine.h"

namespace  atto {
    enum class PlayerCardType {
        AttackSpeedIncrease,
        AttackDamageIncrease,
        AttackAccuracyIncrease,
        AmmoCapacityIncrease,
        ReloadSpeedIncrease,
        MaxHealthIncrease,
        RestoreHealth,
        ExtraCoins_10,
    };

    class PlayerCard {
    public:
        void Initialize();

    private:
        const Texture * front = nullptr;
        const Texture * back = nullptr;
        const Texture * emptyGem = nullptr;
        const Texture * fullGem = nullptr;
    };
}
