#pragma once

#include "game_entity.h"

namespace atto {
    class Entity_HealthPickup : public Entity {
    public:
        Entity_HealthPickup();

        void OnSpawn() override;
        void OnUpdate( f32 dt ) override;
        void OnRender( Renderer & renderer ) override;
        void OnDespawn() override;

        AlignedBox GetBounds() const override;
        bool RayTest( const Vec3 & start, const Vec3 & dir, f32 & dist ) const override;
        void Serialize( Serializer & serializer ) override;

    private:
        const Texture * pickupTexture = nullptr;
        const Texture * sparkleTexture = nullptr;
        SoundCollection sndPickup;

        f32 bobTimer = 0.0f;
        f32 sparkTimer = 0.0f;
        i32 healAmount = 50;

        static constexpr f32 PICKUP_SIZE = 0.35f;
        static constexpr f32 BOB_SPEED = 2.5f;
        static constexpr f32 BOB_AMOUNT = 0.15f;
        static constexpr f32 MAGNET_RADIUS = 3.0f;
        static constexpr f32 COLLECT_RADIUS = 0.5f;
        static constexpr f32 MAGNET_SPEED = 12.0f;
    };
} // atto
