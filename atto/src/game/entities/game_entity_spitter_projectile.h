#pragma once
#include "game_entity.h"

namespace atto {

    class Entity_SpitterProjectile : public Entity {
    public:
        Entity_SpitterProjectile();

        void        OnSpawn() override;
        void        OnUpdate( f32 dt ) override;
        void        OnRender( Renderer & renderer ) override;
        void        OnDespawn() override;
        AlignedBox  GetBounds() const override;

        // Called by the Spitter immediately after CreateEntity/OnSpawn.
        void Launch( const Vec3 & from, const Vec3 & to );

    private:
        static constexpr f32 FLIGHT_DURATION = 1.8f;
        static constexpr f32 ARC_HEIGHT      = 2.5f;  // peak height above the linear path
        static constexpr f32 HIT_RADIUS      = 0.55f; // distance to player for a hit
        static constexpr f32 RENDER_SCALE    = 0.22f;
        static constexpr i32 DAMAGE          = 8;

        const StaticModel * model      = nullptr;
        Vec3                startPos   = Vec3( 0.0f );
        Vec3                endPos     = Vec3( 0.0f );
        f32                 flightTime = 0.0f;
        bool                launched   = false;
    };

} // namespace atto
