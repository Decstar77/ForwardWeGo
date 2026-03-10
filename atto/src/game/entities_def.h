#pragma once
#include "engine/atto_engine.h"

namespace atto {
    enum class UnitType {
        NONE = 0,
        SOL_SCOUT,
        SOL_MARINE,
    };

    struct UnitDef {
        UnitType type;

        fp speed;
        fp health;

        fp radius;

        SmallString spriteName;
        i32 frameWidth;
        i32 frameHeight;
        i32 sheetWidth;
        i32 sheetHeight;
        i32 startFrame;
        i32 frameCount;
    };

    void            InitializeUnitDefs();
    const UnitDef * GetUnitDef( UnitType type );
}