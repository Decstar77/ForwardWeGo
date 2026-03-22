#include "game_entity_gamemode_kill_all.h"
#include "../game_map.h"

namespace atto {
    ATTO_REGISTER_CLASS(  Entity, Entity_GameMode_KillAllEntities, EntityType::GameMode_KillAllEntities )

    Entity_GameMode_KillAllEntities::Entity_GameMode_KillAllEntities() {
        type = EntityType::GameMode_KillAllEntities;
    }

    void Entity_GameMode_KillAllEntities::OnSpawn() {
    }

    void Entity_GameMode_KillAllEntities::OnUpdate( f32 dt ) {
        for ( i32 spawnIdIndex = 0; spawnIdIndex < (i32)remainingEntities.size(); spawnIdIndex++ ) {
            bool missing = true;
            for ( i32 entityIndex = 0; entityIndex < map->GetEntityCount(); entityIndex++ ) {
                const Entity * entity = map->GetEntity( entityIndex );
                if ( entity->GetSpawnId() == remainingEntities[spawnIdIndex] ) {
                    missing = false;
                    continue;
                }
            }

            if ( missing == true ) {
                LOG_INFO( "Entity_GameMode_KillAllEntities :: Removing :: %d", remainingEntities[spawnIdIndex] );
                remainingEntities.erase( remainingEntities.begin() + spawnIdIndex );
                spawnIdIndex--;
            }
        }

        if ( remainingEntities.size() == 0 ) {
            Engine::Get().TransitionToScene( "GameMapScene", mapName.c_str() );
            LOG_INFO( "Entity_GameMode_KillAllEntities :: Game over" );
        }
    }

    void Entity_GameMode_KillAllEntities::OnRender( Renderer & renderer ) {
    }

    void Entity_GameMode_KillAllEntities::OnDespawn() {
    }

    void Entity_GameMode_KillAllEntities::Serialize( Serializer & serializer ) {
        Entity::Serialize( serializer );
        serializer( "MapName", mapName );
        serializer( "RemainingEntities", remainingEntities );
    }
}
