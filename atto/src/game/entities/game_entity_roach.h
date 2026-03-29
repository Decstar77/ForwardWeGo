#pragma once

#include "game_entity_agent.h"

namespace atto {

    class Entity_Roach : public Entity_Agent {
    public:
        Entity_Roach();

        void OnSpawn() override;
        void OnUpdate( f32 dt ) override;
        void OnDespawn() override;

    protected:
        void OnAgentAttack() override;

    private:
        SoundCollection sndAttack;
    };
}
