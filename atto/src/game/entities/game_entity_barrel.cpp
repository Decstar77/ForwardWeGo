#include "game_entity_barrel.h"
#include "../game_map.h"

#include "engine/atto_class_factory.h"

namespace atto {
    ATTO_REGISTER_CLASS( Entity, Entity_Barrel , EntityType::Barrel )

    Entity_Barrel::Entity_Barrel() {
        type = EntityType::Barrel;
    }

    void Entity_Barrel::OnSpawn() {
        Renderer &renderer = Engine::Get().GetRenderer();
        model = renderer.GetOrLoadStaticModel( "assets/player/props/barrel.fbx", 3.5f );
        orientation = Mat3( glm::rotate( glm::mat4( 1 ), glm::radians( -90.0f ), Vec3( 1.0f, 0.0f, 0.0f ) ) );
    }

    void Entity_Barrel::OnUpdate( f32 dt ) {
    }

    void Entity_Barrel::OnRender( Renderer &renderer ) {
        ATTO_ASSERT( model != nullptr, "Model is null" );

        Mat4 modelMatrix = GetModelMatrix();
        renderer.RenderStaticModel( model, modelMatrix );
    }

    void Entity_Barrel::OnDespawn() {
    }

    AlignedBox Entity_Barrel::GetBounds() const {
        ATTO_ASSERT( model != nullptr, "Model is null" );
        AlignedBox bounds = model->GetBounds();
        bounds.Translate( position );
        bounds.RotateAround( position, orientation );
        return bounds;
    }

    bool Entity_Barrel::RayTest( const Vec3 &start, const Vec3 &dir, f32 &dist ) const {
        AlignedBox bounds = GetBounds();
        return Raycast::TestAlignedBox( start, dir, bounds, dist );
    }

    void Entity_Barrel::DebugDrawBounds( Renderer &renderer ) {
        AlignedBox bounds = GetBounds();
        renderer.DebugAlignedBox( bounds );
    }

    void Entity_Barrel::TakeDamage( i32 damage ) {
        health -= damage;
        if ( health <= 0 ) {
            map->DestroyEntity( this );
        }
    };
} // atto
