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
        void TakeDamage( i32 damage ) override;

    private:
        const StaticModel * model = nullptr;
        i32 health = 100;
    };
} // atto


