#include "game_entity_portal.h"

namespace atto {
    ATTO_REGISTER_CLASS( Entity, Entity_Portal, EntityType::Portal )

    Entity_Portal::Entity_Portal() {
        type = EntityType::Portal;
    }

    void Entity_Portal::OnSpawn() {
        Renderer & renderer = Engine::Get().GetRenderer();
        texturePortal = renderer.GetOrLoadTexture( "assets/textures/fx/synty/PolygonParticles_Portal_Single_Chipped.png" );
        textureSparkle = renderer.GetOrLoadTexture( "assets/textures/fx/synty/PolygonParticles_Sparkle.png" );
    }

    void Entity_Portal::OnUpdate( f32 dt ) {
    }

    void Entity_Portal::OnRender( Renderer &renderer ) {

    }
}


