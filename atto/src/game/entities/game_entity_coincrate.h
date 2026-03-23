#pragma once

#include "game_entity.h"

namespace atto {
    class Entity_CoinCrate : public Entity {
    public:
        Entity_CoinCrate();

        void OnSpawn() override;
        void OnRender( Renderer &renderer ) override;
        AlignedBox GetBounds() const override;
        bool RayTest( const Vec3 &start, const Vec3 &dir, f32 &dist ) const override;
        TakeDamageResult TakeDamage( i32 damage ) override;

    private:
        const StaticModel * model = nullptr;
        i32                 health = 100;
        SoundCollection     destroySound;
    };
}

