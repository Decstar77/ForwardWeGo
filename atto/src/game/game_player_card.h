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
            case PlayerCardType::AmmoCapacityIncrease:   return "Perpetual fire";
            case PlayerCardType::ReloadSpeedIncrease:    return "Robotic Hands";
            case PlayerCardType::MaxHealthIncrease:      return "Vitality";
            case PlayerCardType::RestoreHealth:          return "Bandage Up";
            case PlayerCardType::ExtraCoins_10:          return "Jackpot";
            default:                                     return "???";
        }
    }

    inline const char * PlayerCardTypeToDescription( PlayerCardType type ) {
        switch ( type ) {
            case PlayerCardType::AttackSpeedIncrease:    return "+5% Attack Speed";
            case PlayerCardType::AttackDamageIncrease:   return "+5% Damage";
            case PlayerCardType::AttackAccuracyIncrease: return "+10% Accuracy";
            case PlayerCardType::AmmoCapacityIncrease:   return "+2 Ammo Capacity";
            case PlayerCardType::ReloadSpeedIncrease:    return "+5% Reload Speed";
            case PlayerCardType::MaxHealthIncrease:      return "+25 Max Health";
            case PlayerCardType::RestoreHealth:          return "Restore Health";
            case PlayerCardType::ExtraCoins_10:          return "+10 Coins";
            default:                                     return "";
        }
    }
}
