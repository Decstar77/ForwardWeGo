#pragma once

#include "game_entity.h"
#include <string>
#include <vector>

namespace atto {
    class Entity_GameMode_KillAllEntities : public Entity {
    public:
        Entity_GameMode_KillAllEntities();

        void OnSpawn() override;
        void OnUpdate( f32 dt ) override;
        void OnRender( Renderer & renderer ) override;
        void OnDespawn() override;

        void Serialize( Serializer & serializer ) override;

    private:
        std::string mapName = "";
        std::vector<SpawnId> remainingEntities;
    };
}
