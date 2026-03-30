#include "game_entity_spitter.h"
#include "game_entity_spitter_projectile.h"

#include "game/game_map.h"

namespace atto {

    ATTO_REGISTER_CLASS( Entity, Entity_Spitter, EntityType::Spitter )

    Entity_Spitter::Entity_Spitter() {
        type = EntityType::Spitter;
        config.maxHealth = 75;
        config.attackRange = 20.0f;
        config.attackRangeHyst = 20.0f;
        health           = config.maxHealth;
    }

    void Entity_Spitter::OnSpawn() {
        model = Engine::Get().GetRenderer().GetOrLoadStaticModel( "assets/models/sm-declan/SM_Prop_DeadZub_Green.obj" );

        sndAttack.Initialize();
        sndAttack.LoadSounds( {
            "assets/sounds/roach/attack.wav"
        } );

        sndWalk.Initialize();
        sndWalk.LoadSounds( {
            "assets/sounds/roach/walk.wav"
        } );
    }

    void Entity_Spitter::OnUpdate( f32 dt ) {
        AgentOnUpdate( dt );
    }

    void Entity_Spitter::OnDespawn() {
    }

    void Entity_Spitter::OnAgentAttack() {
        // Spawn point is at the spitter's head height
        const Vec3 spawnPos  = position + Vec3( 0.0f, 1.2f, 0.0f );
        const Vec3 playerPos = map->GetPlayerPosition();

        Entity_SpitterProjectile * proj = static_cast<Entity_SpitterProjectile *>(
            map->CreateEntity( EntityType::SpitterProjectile ) );
        if ( proj ) {
            proj->OnSpawn();
            proj->Launch( spawnPos, playerPos );
        }

        sndAttack.PlayAt( position, 15.0f );
    }
}
