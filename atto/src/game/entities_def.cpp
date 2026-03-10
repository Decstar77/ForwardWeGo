#include "entities_def.h"

namespace atto {
    static FixedList<UnitDef, 128> unitDefs;

    void InitializeUnitDefs() {
        unitDefs.Clear();
        unitDefs.AddEmpty();

        {
            UnitDef & scout  = unitDefs.AddEmpty();
            scout.type = UnitType::SOL_SCOUT;
            scout.speed = fp( 80 );
            scout.health = fp( 100 );
            scout.radius = fp( 1 ) / fp( 2 );
            scout.spriteName = "assets/scout/scout_idle_side.png";
            scout.frameWidth = 12;
            scout.frameHeight = 12;
            scout.sheetWidth = 48;
            scout.sheetHeight = 12;
            scout.startFrame = 0;
            scout.frameCount = 4;
        }
    }


    const UnitDef * GetUnitDef( UnitType type ) {
        return &unitDefs[static_cast<i32>(type)];
    }
}