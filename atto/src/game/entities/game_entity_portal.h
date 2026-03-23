#pragma once

#include "game_entity.h"

namespace atto {
    class Entity_Portal : public Entity {
    public:
        Entity_Portal();
        void            OnSpawn() override;
        void            OnUpdate( f32 dt ) override;
        void            OnRender( Renderer &renderer ) override;

    private:
        const Texture * texturePortal = nullptr;
        const Texture * textureSparkle = nullptr;
        f32 portalRotation = 0.0f;
    };
}
