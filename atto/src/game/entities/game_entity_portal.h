#pragma once

#include "game_entity.h"

namespace atto {
    class Entity_Portal : public Entity {
    public:
        Entity_Portal();
        void            OnSpawn() override;
        void            OnDespawn() override;
        void            OnUpdate( f32 dt ) override;
        void            OnRender( Renderer &renderer ) override;
        void            Serialize( Serializer & serializer ) override;
        AlignedBox      GetBounds() const override;
        bool            RayTest( const Vec3 & start, const Vec3 & dir, f32 & dist ) const override;

        void            Activate();

    private:
        bool active = false;

        std::string mapName = "";

        f32 portalRotation = 0.0f;
        f32 sparkTimer = 0.0f;
        f32 portalSize = 2.0f;

        const Texture * texturePortal = nullptr;
        const Texture * textureSparkle = nullptr;

        SoundCollection sndPortalHum;
        SoundCollection sndPortalTavel;
        AudioSourceHandle sndInstancePortalHum = {};
    };
}
