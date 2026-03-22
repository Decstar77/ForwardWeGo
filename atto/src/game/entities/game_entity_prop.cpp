#include "game_entity_prop.h"

namespace atto {
    ATTO_REGISTER_CLASS(  Entity, Entity_Prop, EntityType::Prop )

    Entity_Prop::Entity_Prop() {
        type = EntityType::Prop;
    }

    void Entity_Prop::OnSpawn() {
    }

    void Entity_Prop::OnUpdate( f32 dt ) {
    }

    void Entity_Prop::OnRender( Renderer & renderer ) {
        if ( model != nullptr ) {
            Mat4 modelMatrix = GetModelMatrix();
            renderer.RenderStaticModel( model, modelMatrix );
        }
    }

    void Entity_Prop::OnDespawn() {
    }

    AlignedBox Entity_Prop::GetBounds() const {
        if ( model == nullptr ) {
            return {};
        }

        AlignedBox bounds = model->GetBounds();
        bounds.Translate( position );
        bounds.RotateAround( position, orientation );
        return bounds;
    }

    bool Entity_Prop::RayTest( const Vec3 & start, const Vec3 & dir, f32 & dist ) const {
        AlignedBox bounds = GetBounds();
        return Raycast::TestAlignedBox( start, dir, bounds, dist );
    }

    void Entity_Prop::DebugDrawBounds( Renderer & renderer ) {
        AlignedBox bounds = GetBounds();
        renderer.DebugAlignedBox( bounds );
    }

    void Entity_Prop::Serialize( Serializer & serializer ) {
        Entity::Serialize( serializer );
        serializer( "model", model );
    }
}
