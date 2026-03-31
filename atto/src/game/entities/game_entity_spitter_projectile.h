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
        static constexpr f32 FLIGHT_DURATION = 1.0f;
        static constexpr f32 ARC_HEIGHT      = 1.0f;  // peak height above the linear path
        static constexpr f32 HIT_RADIUS      = 1.25f; // distance to player for a hit
        static constexpr f32 RENDER_SCALE    = 0.22f;
        static constexpr i32 DAMAGE          = 8;
        static constexpr f32 EMIT_INTERVAL   = 0.03f; // seconds between trail emissions

        const Texture *     txtBubble1 = nullptr;
        const Texture *     txtBubble2 = nullptr;
        const Texture *     txtFumes   = nullptr;
        Vec3                startPos   = Vec3( 0.0f );
        Vec3                endPos     = Vec3( 0.0f );
        f32                 flightTime = 0.0f;
        f32                 emitTimer  = 0.0f;
        bool                launched   = false;

        void EmitTrailParticles();
        void EmitImpactBurst();
    };

} // namespace atto
