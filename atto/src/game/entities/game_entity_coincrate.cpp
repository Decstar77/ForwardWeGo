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
            "assets/sounds/resources/coins1.wav",
            "assets/sounds/resources/coins2.wav"
        } );
    }

    void Entity_CoinCrate::OnRender( Renderer &renderer ) {
        renderer.RenderStaticModel( model, GetModelMatrix() );

        // Health bar above entity
        if ( health < MAX_HEALTH ) {
            AlignedBox bounds = GetBounds();
            f32 barY = bounds.max.y + 0.3f;
            Vec3 barPos = Vec3( position.x, barY, position.z );

            Vec3 cameraPos = map->GetPlayerPosition();
            Vec3 cameraUp = map->GetPlayerCameraUp();
            if ( LengthSquared( cameraUp ) < 0.001f ) {
                cameraUp = Vec3( 0.0f, 1.0f, 0.0f );
            }

            f32 frac = static_cast<f32>( health ) / static_cast<f32>( MAX_HEALTH );
            Vec4 fillColor = frac > 0.5f
                ? Vec4( 0.1f, 0.8f, 0.1f, 0.9f )
                : frac > 0.25f
                    ? Vec4( 1.0f, 0.7f, 0.0f, 0.9f )
                    : Vec4( 0.9f, 0.1f, 0.1f, 0.9f );

            renderer.RenderWorldBar( barPos, 0.8f, 0.08f, frac, fillColor, cameraPos, cameraUp );
        }
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

            RNG & rng = Engine::Get().GetRNG();
            for ( int i = 0; i < coinGiveAmount; i++ ) {
                Entity * coin = map->CreateEntity( EntityType::Coin );
                if ( coin ) {
                    const float x = rng.Float() * 1.2f;
                    const float z = rng.Float() * 1.2f;
                    coin->SetPosition( position + Vec3( x, 0.5f, z ) );
                    coin->OnSpawn();
                }
            }
        }
        return TakeDamageResult::Success_HP;
    }

    void Entity_CoinCrate::Serialize( Serializer &serializer ) {
        Entity::Serialize( serializer );
        serializer("CoinGiveAmount", coinGiveAmount);
    }
}

