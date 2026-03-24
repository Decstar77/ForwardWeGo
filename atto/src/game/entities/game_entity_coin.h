#pragma once

#include "game_entity.h"

namespace atto {
    class Entity_Coin : public Entity {
    public:
        Entity_Coin();

        void OnSpawn() override;
        void OnUpdate( f32 dt ) override;
        void OnRender( Renderer & renderer ) override;
        void OnDespawn() override;

        AlignedBox GetBounds() const override;
        bool RayTest( const Vec3 & start, const Vec3 & dir, f32 & dist ) const override;
        void Serialize( Serializer & serializer ) override;

    private:
        const Texture * coinTexture = nullptr;
        const Texture * sparkleTexture = nullptr;
        SoundCollection sndCollection;

        f32 bobTimer = 0.0f;
        f32 rotation = 0.0f;
        f32 sparkTimer = 0.0f;
        i32 coinValue = 1;

        static constexpr f32 COIN_SIZE = 0.2f;
        static constexpr f32 BOB_SPEED = 3.0f;
        static constexpr f32 BOB_AMOUNT = 0.15f;
        static constexpr f32 SPIN_SPEED = 2.5f;
        static constexpr f32 MAGNET_RADIUS = 5.0f;
        static constexpr f32 COLLECT_RADIUS = 0.5f;
        static constexpr f32 MAGNET_SPEED = 15.0f;
    };
} // atto
