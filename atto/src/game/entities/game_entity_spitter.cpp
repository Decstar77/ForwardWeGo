#include "game_entity_spitter.h"

#include "game/game_map.h"

namespace atto {

    ATTO_REGISTER_CLASS( Entity, Entity_Spitter, EntityType::Spitter )

    Entity_Spitter::Entity_Spitter() {
        type = EntityType::Spitter;
        // TODO: tune spitter-specific config values
        config.maxHealth = 75;
        health           = config.maxHealth;
    }

    void Entity_Spitter::OnSpawn() {
        // TODO: load spitter model
        // model = Engine::Get().GetRenderer().GetOrLoadStaticModel( "assets/models/sm/SM_Spitter.obj" );

        sndAttack.Initialize();
        // TODO: load spitter attack sound
        // sndAttack.LoadSounds( { "assets/sounds/spitter/attack.wav" } );

        sndWalk.Initialize();
        // TODO: load spitter walk sound
        // sndWalk.LoadSounds( { "assets/sounds/spitter/walk.wav" } );
    }

    void Entity_Spitter::OnUpdate( f32 dt ) {
        AgentOnUpdate( dt );
    }

    void Entity_Spitter::OnDespawn() {
    }

    void Entity_Spitter::OnAgentAttack() {
        // TODO: spawn projectile instead of dealing direct melee damage
        map->DamagePlayer( 8 );
        sndAttack.PlayAt( position, 15.0f );
    }
}
