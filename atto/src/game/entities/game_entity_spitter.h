#pragma once

#include "game_entity.h"

namespace atto {
    class Entity_Spitter : public Entity {
    public:
        Entity_Spitter();

        void OnSpawn() override;
        void OnUpdate( f32 dt ) override;
        void OnRender( Renderer &renderer ) override;
        void OnDespawn() override;

        AlignedBox GetBounds() const override;
        Box GetCollider() const override;
        bool RayTest( const Vec3 &start, const Vec3 &dir, f32 &dist ) const override;

        void Serialize( Serializer &serializer ) override;

        TakeDamageResult TakeDamage( i32 damage ) override;
    };
} // atto
