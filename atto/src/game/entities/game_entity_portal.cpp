#include "game_entity_portal.h"
#include "../game_map.h"

namespace atto {
    ATTO_REGISTER_CLASS( Entity, Entity_Portal, EntityType::Portal )

    Entity_Portal::Entity_Portal() {
        type = EntityType::Portal;
    }

    void Entity_Portal::OnSpawn() {
        Renderer & renderer = Engine::Get().GetRenderer();
        texturePortal = renderer.GetOrLoadTexture( "assets/textures/fx/synty/PolygonParticles_Portal_Single_Chipped.png" );
        textureSparkle = renderer.GetOrLoadTexture( "assets/textures/fx/synty/PolygonParticles_Sparkle.png" );
        sndPortalHum.Initialize();
        sndPortalHum.LoadSounds( {
            "assets/sounds/door/portal-hum02.wav"
        }, true );
        sndPortalTavel.Initialize();
        sndPortalTavel.LoadSounds( {
            "assets/sounds/door/portal-travel.wav"
        } );
    }

    void Entity_Portal::OnDespawn() {
        Engine::Get().GetAudioSystem().Stop( sndInstancePortalHum );
    }

    void Entity_Portal::OnUpdate( f32 dt ) {
        if ( active == false ) {
            return;
        }

        portalRotation += 1.5f * dt;

        // Emit spark particles around the portal
        sparkTimer -= dt;
        if ( sparkTimer <= 0.0f ) {
            sparkTimer = 0.05f;

            f32 angle = RandomFloat() * 6.2831853f;
            f32 radius = portalSize * 0.5f * ( 0.8f + RandomFloat() * 0.4f );
            Vec3 offset = Vec3( cosf( angle ) * radius, sinf( angle ) * radius, 0.0f );

            ParticleParms parms = {};
            parms.position = position + offset;
            parms.positionVariance = Vec3( 0.1f );
            parms.velocity = Vec3( 0.0f, 0.5f + RandomFloat() * 0.5f, 0.0f );
            parms.velocityVariance = Vec3( 0.2f, 0.2f, 0.2f );
            parms.gravity = Vec3( 0.0f, 0.0f, 0.0f );
            parms.lifetime = 0.6f;
            parms.lifetimeVariance = 0.3f;
            parms.startSize = 0.15f + RandomFloat() * 0.1f;
            parms.endSize = 0.0f;
            parms.startColor = Color( 0.4f, 0.6f, 1.0f, 1.0f );
            parms.endColor = Color( 0.1f, 0.2f, 1.0f, 0.0f );
            parms.texture = textureSparkle;
            parms.count = 1;

            map->GetParticleSystem().Emit( parms );
        }

        const AlignedBox warpArea = GetBounds();
        const Vec3 playerPos = map->GetPlayerPosition();
        const Vec3 closePoint = warpArea.ClosestPoint( playerPos );
        const f32 dist = Distance( playerPos, closePoint );
        const f32 threshold = 0.5f;
        if ( dist < threshold ) {
            if ( mapName.empty() == false ) {
                sndPortalTavel.Play( 0.5f );
                Engine::Get().TransitionToScene( "GameMapScene", mapName.c_str() );
            } else {
                LOG_WARN( "Map portal has no map set" );
            }
        }
    }

    void Entity_Portal::OnRender( Renderer & renderer ) {
        Vec3 cameraPos = map->GetPlayerPosition();
        Vec3 cameraUp = map->GetPlayerCameraUp();

        // If camera up is zero (e.g. in editor), use world up
        if ( LengthSquared( cameraUp ) < 0.001f ) {
            cameraUp = Vec3( 0.0f, 1.0f, 0.0f );
        }

        Vec4 blueColor = Vec4( 0.3f, 0.5f, 1.0f, 0.9f );
        renderer.RenderBillboard( texturePortal, position, portalSize, cameraPos, cameraUp, portalRotation, blueColor );
    }

    void Entity_Portal::Serialize( Serializer & serializer ) {
        Entity::Serialize( serializer );
        serializer( "PortalSize", portalSize );
        serializer("MapName", mapName );
    }

    AlignedBox Entity_Portal::GetBounds() const {
        f32 half = portalSize * 0.5f;
        AlignedBox box;
        box.min = position - Vec3( half, half, half );
        box.max = position + Vec3( half, half, half );
        return box;
    }

    bool Entity_Portal::RayTest( const Vec3 & start, const Vec3 & dir, f32 & dist ) const {
        AlignedBox bounds = GetBounds();
        return Raycast::TestAlignedBox( start, dir, bounds, dist );
    }

    void Entity_Portal::Activate() {
        if ( active == false ) {
            active = true;
            sndInstancePortalHum = sndPortalHum.PlayAt( position, 0.5f, true );
        }
    }

}
