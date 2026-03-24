#include "game_entity_coin.h"
#include "../game_map.h"
#include "../game_global_state.h"

#include "engine/atto_class_factory.h"

namespace atto {
    ATTO_REGISTER_CLASS( Entity, Entity_Coin, EntityType::Coin )

    Entity_Coin::Entity_Coin() {
        type = EntityType::Coin;
        isCollidable = false;
    }

    void Entity_Coin::OnSpawn() {
        Renderer & renderer = Engine::Get().GetRenderer();
        coinTexture = renderer.GetOrLoadTexture( "assets/textures/coin.png" );
        sparkleTexture = renderer.GetOrLoadTexture( "assets/textures/fx/synty/PolygonParticles_Sparkle.png" );
        bobTimer = RandomFloat() * 6.2831853f;

        sndCollection.Initialize();
        sndCollection.LoadSounds( {
            "resources/coins1.wav",
            "resources/coins2.wav",
        } );
    }

    void Entity_Coin::OnUpdate( f32 dt ) {
        bobTimer += BOB_SPEED * dt;
        rotation += SPIN_SPEED * dt;

        // Sparkle particles
        sparkTimer -= dt;
        if ( sparkTimer <= 0.0f ) {
            sparkTimer = 0.15f;

            ParticleParms parms = {};
            parms.position = position;
            parms.positionVariance = Vec3( 0.2f );
            parms.velocity = Vec3( 0.0f, 0.5f, 0.0f );
            parms.velocityVariance = Vec3( 0.3f, 0.3f, 0.3f );
            parms.gravity = Vec3( 0.0f, 0.0f, 0.0f );
            parms.lifetime = 0.4f;
            parms.lifetimeVariance = 0.2f;
            parms.startSize = 0.1f;
            parms.endSize = 0.0f;
            parms.startColor = Color( 1.0f, 0.85f, 0.0f, 0.8f );
            parms.endColor = Color( 1.0f, 0.6f, 0.0f, 0.0f );
            parms.texture = sparkleTexture;
            parms.count = 1;

            map->GetParticleSystem().Emit( parms );
        }

        // Magnet towards player
        Vec3 playerPos = map->GetPlayerPosition();
        Vec3 toPlayer = playerPos - position;
        f32 dist = glm::length( toPlayer );

        if ( dist < COLLECT_RADIUS ) {
            sndCollection.PlayAt( position );
            GameGlobalState::Get().AddPlayerCoins( coinValue );
            map->DestroyEntity( this );
            return;
        }

        if ( dist < MAGNET_RADIUS ) {
            Vec3 dir = toPlayer / dist;
            f32 speed = MAGNET_SPEED * ( 1.0f - dist / MAGNET_RADIUS );
            position += dir * speed * dt;
        }
    }

    void Entity_Coin::OnRender( Renderer & renderer ) {
        Vec3 cameraPos = map->GetPlayerPosition();
        Vec3 cameraUp = map->GetPlayerCameraUp();

        if ( LengthSquared( cameraUp ) < 0.001f ) {
            cameraUp = Vec3( 0.0f, 1.0f, 0.0f );
        }

        Vec3 renderPos = position;
        renderPos.y += sinf( bobTimer ) * BOB_AMOUNT;

        renderer.RenderBillboard( coinTexture, renderPos, COIN_SIZE, cameraPos, cameraUp, rotation );
    }

    void Entity_Coin::OnDespawn() {
        // Collection burst particles
        ParticleParms burst = {};
        burst.position = position;
        burst.positionVariance = Vec3( 0.1f );
        burst.velocity = Vec3( 0.0f, 2.0f, 0.0f );
        burst.velocityVariance = Vec3( 2.0f, 1.5f, 2.0f );
        burst.gravity = Vec3( 0.0f, -3.0f, 0.0f );
        burst.drag = 2.0f;
        burst.lifetime = 0.5f;
        burst.lifetimeVariance = 0.2f;
        burst.startSize = 0.15f;
        burst.endSize = 0.0f;
        burst.startColor = Color( 1.0f, 0.9f, 0.2f, 1.0f );
        burst.endColor = Color( 1.0f, 0.6f, 0.0f, 0.0f );
        burst.texture = sparkleTexture;
        burst.count = 10;

        map->GetParticleSystem().Emit( burst );
    }

    AlignedBox Entity_Coin::GetBounds() const {
        const f32 half = COIN_SIZE * 0.5f;
        AlignedBox box = {};
        box.min = position - Vec3( half, half, half );
        box.max = position + Vec3( half, half, half );
        return box;
    }

    bool Entity_Coin::RayTest( const Vec3 & start, const Vec3 & dir, f32 & dist ) const {
        AlignedBox bounds = GetBounds();
        return Raycast::TestAlignedBox( start, dir, bounds, dist );
    }

    void Entity_Coin::Serialize( Serializer & serializer ) {
        Entity::Serialize( serializer );
        serializer( "CoinValue", coinValue );
    }
} // atto
