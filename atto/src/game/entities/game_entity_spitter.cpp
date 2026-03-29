#include "game_entity_spitter.h"

namespace atto {
    Entity_Spitter::Entity_Spitter() {
    }

    void Entity_Spitter::OnSpawn() {
        Entity::OnSpawn();
    }

    void Entity_Spitter::OnUpdate( f32 dt ) {
        Entity::OnUpdate( dt );
    }

    void Entity_Spitter::OnRender( Renderer &renderer ) {
        Entity::OnRender( renderer );
    }

    void Entity_Spitter::OnDespawn() {
        Entity::OnDespawn();
    }

    AlignedBox Entity_Spitter::GetBounds() const {
        return Entity::GetBounds();
    }

    Box Entity_Spitter::GetCollider() const {
        return Entity::GetCollider();
    }

    bool Entity_Spitter::RayTest( const Vec3 &start, const Vec3 &dir, f32 &dist ) const {
        return Entity::RayTest( start, dir, dist );
    }

    void Entity_Spitter::Serialize( Serializer &serializer ) {
        Entity::Serialize( serializer );
    }

    TakeDamageResult Entity_Spitter::TakeDamage( i32 damage ) {
        return Entity::TakeDamage( damage );
    }
}
