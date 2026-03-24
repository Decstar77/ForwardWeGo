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
        model = renderer.GetOrLoadStaticModel( "assets/player/props/barrel.fbx", 3.0f );
        orientation = Mat3( glm::rotate( glm::mat4( 1 ), glm::radians( -90.0f ), Vec3( 1.0f, 0.0f, 0.0f ) ) );
        particleSmokeTexture = renderer.GetOrLoadTexture( "assets/textures/fx/synty/PolygonParticles_Smoke_01.png" );
        particleSparkleTexture = renderer.GetOrLoadTexture( "assets/textures/fx/synty/PolygonParticles_Sparkle.png" );
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

    TakeDamageResult Entity_Barrel::TakeDamage( i32 damage ) {
        health -= damage;
        if ( health <= 0 ) {
            Explode();
            map->DestroyEntity( this );
        }

        return TakeDamageResult::Success_HP;
    }

    void Entity_Barrel::Explode() {
        ParticleSystem & ps = map->GetParticleSystem();

        // Fireball — large, bright, expanding
        ParticleParms fireball;
        fireball.position         = position + Vec3( 0.0f, 0.5f, 0.0f );
        fireball.positionVariance = Vec3( 0.3f );
        fireball.velocity         = Vec3( 0.0f, 2.0f, 0.0f );
        fireball.velocityVariance = Vec3( 1.5f, 2.0f, 1.5f );
        fireball.gravity          = Vec3( 0.0f, 1.0f, 0.0f );
        fireball.drag             = 2.0f;
        fireball.lifetime         = 0.6f;
        fireball.lifetimeVariance = 0.2f;
        fireball.startSize        = 1.0f;
        fireball.endSize          = 3.0f;
        fireball.startColor       = Color( 1.0f, 0.6f, 0.1f, 1.0f );
        fireball.endColor         = Color( 1.0f, 0.2f, 0.0f, 0.0f );
        fireball.texture          = particleSparkleTexture;
        fireball.count            = 20;
        ps.Emit( fireball );

        // Smoke plume — dark, rises, expands
        ParticleParms smoke;
        smoke.position         = position + Vec3( 0.0f, 0.5f, 0.0f );
        smoke.positionVariance = Vec3( 0.5f );
        smoke.velocity         = Vec3( 0.0f, 3.0f, 0.0f );
        smoke.velocityVariance = Vec3( 1.0f, 1.5f, 1.0f );
        smoke.gravity          = Vec3( 0.0f, 0.5f, 0.0f );
        smoke.drag             = 1.5f;
        smoke.lifetime         = 1.5f;
        smoke.lifetimeVariance = 0.5f;
        smoke.startSize        = 0.8f;
        smoke.endSize          = 3.5f;
        smoke.startColor       = Color( 0.4f, 0.4f, 0.4f, 0.8f );
        smoke.endColor         = Color( 0.2f, 0.2f, 0.2f, 0.0f );
        smoke.texture          = particleSmokeTexture;
        smoke.count            = 8;
        ps.Emit( smoke );

        // Hot sparks — fast, small, scatter outward
        ParticleParms sparks;
        sparks.position         = position + Vec3( 0.0f, 0.5f, 0.0f );
        sparks.positionVariance = Vec3( 0.1f );
        sparks.velocity         = Vec3( 0.0f, 4.0f, 0.0f );
        sparks.velocityVariance = Vec3( 5.0f, 5.0f, 5.0f );
        sparks.gravity          = Vec3( 0.0f, -9.8f, 0.0f );
        sparks.drag             = 0.5f;
        sparks.lifetime         = 0.8f;
        sparks.lifetimeVariance = 0.3f;
        sparks.startSize        = 0.08f;
        sparks.endSize          = 0.01f;
        sparks.startColor       = Color( 1.0f, 0.9f, 0.3f, 1.0f );
        sparks.endColor         = Color( 1.0f, 0.3f, 0.0f, 0.0f );
        sparks.texture          = nullptr;
        sparks.velocityAligned  = true;
        sparks.stretchFactor    = 3.0f;
        sparks.count            = 25;
        ps.Emit( sparks );

        // Damage nearby entities
        const i32 entityCount = map->GetEntityCount();
        for ( i32 i = 0; i < entityCount; i++ ) {
            Entity * other = map->GetEntity( i );
            if ( other == this || other->IsPendingDestroy() ) {
                continue;
            }

            Vec3 toOther = other->GetPosition() - position;
            f32 dist = glm::length( toOther );
            if ( dist < EXPLOSION_RADIUS ) {
                f32 falloff = 1.0f - ( dist / EXPLOSION_RADIUS );
                i32 dmg = static_cast<i32>( EXPLOSION_DAMAGE * falloff );
                if ( dmg > 0 ) {
                    other->TakeDamage( dmg );
                }
            }
        }

        // Damage player if in range
        Vec3 playerPos = map->GetPlayerPosition();
        f32 distToPlayer = glm::length( playerPos - position );
        if ( distToPlayer < EXPLOSION_RADIUS ) {
            f32 falloff = 1.0f - ( distToPlayer / EXPLOSION_RADIUS );
            i32 dmg = static_cast<i32>( EXPLOSION_DAMAGE * falloff );
            if ( dmg > 0 ) {
                map->DamagePlayer( dmg );
            }
        }

        LOG_INFO( "Barrel exploded at (%.1f, %.1f, %.1f)", position.x, position.y, position.z );
    }
} // atto
