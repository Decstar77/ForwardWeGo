#include "game_entity_exit_door.h"

namespace atto {
    Entity_ExitDoor::Entity_ExitDoor() {
        type = EntityType::ExitDoor;
    }

    void Entity_ExitDoor::OnSpawn() {
        Renderer & renderer = Engine::Get().GetRenderer();
        modelClosed = renderer.GetOrLoadStaticModel( "assets/models/sm-declan/SM_Bld_Section_Door_06_Closed.obj" );
    }

    void Entity_ExitDoor::OnUpdate( f32 dt ) {
    }

    void Entity_ExitDoor::OnRender( Renderer & renderer ) {
        Mat4 modelMatrix = GetModelMatrix();
        renderer.RenderStaticModel( modelClosed, modelMatrix );
    }

    void Entity_ExitDoor::OnDespawn() {
    }

    AlignedBox Entity_ExitDoor::GetBounds() const {
        AlignedBox bounds = modelClosed->GetBounds();
        bounds.Translate( position );
        bounds.RotateAround( position, orientation );
        return bounds;
    }

    bool Entity_ExitDoor::RayTest( const Vec3 & start, const Vec3 & dir, f32 & dist ) const {
        AlignedBox bounds = GetBounds();
        return Raycast::TestAlignedBox( start, dir, bounds, dist );
    }

    void Entity_ExitDoor::DebugDrawBounds( Renderer & renderer ) {
        AlignedBox bounds = GetBounds();
        renderer.DebugAlignedBox( bounds );
    }
}
