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

        PlayerCardTypeCount
    };

    static constexpr i32 PlayerCardTypeCountValue = static_cast<i32>( PlayerCardType::PlayerCardTypeCount );

    inline const char * PlayerCardTypeToName( PlayerCardType type ) {
        switch ( type ) {
            case PlayerCardType::AttackSpeedIncrease:    return "Rapid Fire";
            case PlayerCardType::AttackDamageIncrease:   return "Sharp Rounds";
            case PlayerCardType::AttackAccuracyIncrease: return "Steady Aim";
            case PlayerCardType::AmmoCapacityIncrease:   return "Deep Pockets";
            case PlayerCardType::ReloadSpeedIncrease:    return "Quick Hands";
            case PlayerCardType::MaxHealthIncrease:      return "Vitality";
            case PlayerCardType::RestoreHealth:          return "Medkit";
            case PlayerCardType::ExtraCoins_10:          return "Jackpot";
            default:                                     return "???";
        }
    }

    inline const char * PlayerCardTypeToDescription( PlayerCardType type ) {
        switch ( type ) {
            case PlayerCardType::AttackSpeedIncrease:    return "+15% Attack Speed";
            case PlayerCardType::AttackDamageIncrease:   return "+10% Damage";
            case PlayerCardType::AttackAccuracyIncrease: return "+20% Accuracy";
            case PlayerCardType::AmmoCapacityIncrease:   return "+4 Magazine Size";
            case PlayerCardType::ReloadSpeedIncrease:    return "+20% Reload Speed";
            case PlayerCardType::MaxHealthIncrease:      return "+25 Max Health";
            case PlayerCardType::RestoreHealth:          return "Restore Full Health";
            case PlayerCardType::ExtraCoins_10:          return "+10 Coins";
            default:                                     return "";
        }
    }

    class PlayerCard {
    public:
        void Initialize();

        const Texture * GetFront()    const { return front; }
        const Texture * GetEmptyGem() const { return emptyGem; }
        const Texture * GetFullGem()  const { return fullGem; }

    private:
        const Texture * front = nullptr;
        const Texture * emptyGem = nullptr;
        const Texture * fullGem = nullptr;
    };
}
