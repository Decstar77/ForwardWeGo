#include "game_entity_roach.h"

#include "game/game_map.h"

namespace atto {

    ATTO_REGISTER_CLASS( Entity, Entity_Roach, EntityType::Roach )

    static constexpr i32 RoachAttackDamage = 10;

    Entity_Roach::Entity_Roach() {
        type = EntityType::Roach;
        config.moveSpeed      = 2.8f;
        config.arrivalDist    = 0.65f;
        config.yawSpeed       = 6.0f;
        config.detectionRange = 50.0f;
        config.attackRange    = 2.0f;
        config.attackRangeHyst = 2.0f;
        config.attackRate     = 1.0f;
        config.lostSightTime  = 3.0f;
        config.rePathInterval = 1.5f;
        config.avoidRadius    = 0.75f;
        config.avoidStrength  = 3.0f;
        config.maxHealth      = 100;
        health                = config.maxHealth;
    }

    void Entity_Roach::OnSpawn() {
        model = Engine::Get().GetRenderer().GetOrLoadStaticModel( "assets/models/sm/SM_Prop_DeadZub_01.obj" );

        sndAttack.Initialize();
        sndAttack.LoadSounds( {
            "assets/sounds/roach/attack.wav"
        } );

        sndWalk.Initialize();
        sndWalk.LoadSounds( {
            "assets/sounds/roach/walk.wav"
        } );
    }

    void Entity_Roach::OnUpdate( f32 dt ) {
        AgentOnUpdate( dt );
    }

    void Entity_Roach::OnDespawn() {
    }

    void Entity_Roach::OnAgentAttack() {
        map->DamagePlayer( RoachAttackDamage );
        sndAttack.PlayAt( position, 10.0f );
    }
}
