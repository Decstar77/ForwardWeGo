#pragma once

#include "game_entity.h"

namespace atto {
    class Entity_Barrel : public Entity {
    public:
        Entity_Barrel();

        void OnSpawn() override;
        void OnUpdate( f32 dt ) override;
        void OnRender( Renderer & renderer ) override;
        void OnDespawn() override;

        AlignedBox GetBounds() const override;
        bool RayTest( const Vec3 & start, const Vec3 & dir, f32 & dist ) const override;
        void DebugDrawBounds( Renderer & renderer ) override;
        TakeDamageResult TakeDamage( i32 damage ) override;

    private:
        void Explode();

        const StaticModel * model = nullptr;
        const Texture * particleSmokeTexture = nullptr;
        const Texture * particleSparkleTexture = nullptr;
        i32 health = 100;

        static constexpr f32 EXPLOSION_RADIUS = 8.0f;
        static constexpr i32 EXPLOSION_DAMAGE = 100;
    };
} // atto


