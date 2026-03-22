#pragma once

#include "game_entity.h"

namespace atto {
    class Entity_ExitDoor : public Entity {
    public:
        Entity_ExitDoor();

        void OnSpawn() override;
        void OnUpdate( f32 dt ) override;
        void OnRender( Renderer & renderer ) override;
        void OnDespawn() override;

        AlignedBox GetBounds() const override;
        bool RayTest( const Vec3 & start, const Vec3 & dir, f32 & dist ) const override;
        void DebugDrawBounds( Renderer & renderer ) override;

    private:
        const StaticModel * modelClosed = nullptr;
        const StaticModel * modelOpen = nullptr;
    };
}
