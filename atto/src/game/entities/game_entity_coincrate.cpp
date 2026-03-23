#include "game_entity_coincrate.h"

#include "game/game_map.h"

namespace atto {
    ATTO_REGISTER_CLASS( Entity, Entity_CoinCrate, EntityType::CoinCrate )

    Entity_CoinCrate::Entity_CoinCrate() {
        type = EntityType::CoinCrate;
    }

    void Entity_CoinCrate::OnSpawn() {
        Entity::OnSpawn();
        model = Engine::Get().GetRenderer().GetOrLoadStaticModel( "assets/models/sm/SM_Prop_Crate_02.obj" );
        destroySound.Initialize();
        destroySound.LoadSounds( {
            //"impacts/metal_sheet.wav"
            "resources/coins1.wav",
            "resources/coins2.wav"
        } );
    }

    void Entity_CoinCrate::OnRender( Renderer &renderer ) {
        renderer.RenderStaticModel( model, GetModelMatrix() );
    }

    AlignedBox Entity_CoinCrate::GetBounds() const {
        if ( model != nullptr ) {
            AlignedBox bounds = model->GetBounds();
            bounds.Translate( position );
            bounds.RotateAround( position, orientation );
            return bounds;
        }
        return {};
    }

    bool Entity_CoinCrate::RayTest( const Vec3 &start, const Vec3 &dir, f32 &dist ) const {
        const AlignedBox bounds = GetBounds();
        return Raycast::TestAlignedBox( start, dir, bounds, dist );
    }

    TakeDamageResult Entity_CoinCrate::TakeDamage( i32 damage ) {
        health -= damage;
        if ( health <= 0 ) {
            map->DestroyEntity( this );
            destroySound.PlayAt( position );
        }
        return TakeDamageResult::Success_HP;
    }
}

