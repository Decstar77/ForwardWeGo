#include "game_entity_exit_door.h"

namespace atto {
    ATTO_REGISTER_CLASS(  Entity, Entity_ExitDoor, EntityType::ExitDoor )

    Entity_ExitDoor::Entity_ExitDoor() {
        type = EntityType::ExitDoor;
    }

    void Entity_ExitDoor::OnSpawn() {
        Renderer & renderer = Engine::Get().GetRenderer();
        modelClosed = renderer.GetOrLoadStaticModel( "assets/models/sm-declan/SM_Bld_Section_Door_06_Closed.obj" );
        modelOpen = renderer.GetOrLoadStaticModel( "assets/models/sm-declan/SM_Bld_Section_Door_06_Open.obj" );
        doorSound.Initialize();
        doorSound.LoadSounds( {
            "door/main-door-open.wav"
        } );
    }

    void Entity_ExitDoor::OnUpdate( f32 dt ) {
    }

    void Entity_ExitDoor::OnRender( Renderer & renderer ) {
        const Mat4 modelMatrix = GetModelMatrix();
        if ( isOpen ) {
            renderer.RenderStaticModel( modelOpen, modelMatrix );
        } else {
            renderer.RenderStaticModel( modelClosed, modelMatrix );
        }
    }

    void Entity_ExitDoor::OnDespawn() {
    }

    AlignedBox Entity_ExitDoor::GetBounds() const {
        if ( isOpen == false ) {
            AlignedBox bounds = modelClosed->GetBounds();
            bounds.Translate( position );
            bounds.RotateAround( position, orientation );
            return bounds;
        }

        return {};
    }

    bool Entity_ExitDoor::RayTest( const Vec3 & start, const Vec3 & dir, f32 & dist ) const {
        AlignedBox bounds = GetBounds();
        return Raycast::TestAlignedBox( start, dir, bounds, dist );
    }

    void Entity_ExitDoor::DebugDrawBounds( Renderer & renderer ) {
        AlignedBox bounds = GetBounds();
        renderer.DebugAlignedBox( bounds );
    }

    void Entity_ExitDoor::SetOpen( bool open ) {
        if ( isOpen != open ) {
            isOpen = open;
            doorSound.PlayAt( position, 10 );
        }
    }
}
