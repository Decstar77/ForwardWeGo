#include "game_entity_spitter_projectile.h"
#include "game/game_map.h"

namespace atto {

    ATTO_REGISTER_CLASS( Entity, Entity_SpitterProjectile, EntityType::SpitterProjectile )

    Entity_SpitterProjectile::Entity_SpitterProjectile() {
        type         = EntityType::SpitterProjectile;
        isCollidable = false;
    }

    void Entity_SpitterProjectile::OnSpawn() {
        txtBubble1 = Engine::Get().GetRenderer().GetOrLoadTexture( "assets/textures/fx/synty/PolygonParticles_Bubble.png" );
        txtBubble2 = Engine::Get().GetRenderer().GetOrLoadTexture( "assets/textures/fx/synty/PolygonParticles_Bubble_02.png" );
        txtFumes   = Engine::Get().GetRenderer().GetOrLoadTexture( "assets/textures/fx/synty/PolygonParticles_Fumes_02.png" );
    }

    void Entity_SpitterProjectile::Launch( const Vec3 & from, const Vec3 & to ) {
        startPos   = from;
        endPos     = to;
        position   = from;
        flightTime = 0.0f;
        launched   = true;
    }

    void Entity_SpitterProjectile::OnUpdate( f32 dt ) {
        if ( !launched ) {
            return;
        }

        flightTime += dt;

        const f32 t = glm::clamp( flightTime / FLIGHT_DURATION, 0.0f, 1.0f );

        // Parametric parabola: horizontal lerp + vertical arc
        const Vec3 linearPos = glm::mix( startPos, endPos, t );
        const f32  arc       = ARC_HEIGHT * 4.0f * t * ( 1.0f - t );
        position = linearPos + Vec3( 0.0f, arc, 0.0f );

        // Emit trail particles at regular intervals
        emitTimer += dt;
        if ( emitTimer >= EMIT_INTERVAL ) {
            emitTimer -= EMIT_INTERVAL;
            EmitTrailParticles();
        }

        // Check player hit every frame
        const Vec3 playerPos = map->GetPlayerPosition();
        if ( glm::length( playerPos - position ) < HIT_RADIUS ) {
            EmitImpactBurst();
            map->DamagePlayer( DAMAGE );
            map->DestroyEntity( this );
            return;
        }

        // Destroy when arc completes
        if ( t >= 1.0f ) {
            EmitImpactBurst();
            map->DestroyEntity( this );
        }
    }

    void Entity_SpitterProjectile::OnRender( Renderer & renderer ) {
        (void)renderer;
    }

    void Entity_SpitterProjectile::OnDespawn() {
    }

    void Entity_SpitterProjectile::EmitTrailParticles() {
        ParticleSystem & ps = map->GetParticleSystem();

        // Core poison glob — large bubble
        ParticleParms core = {};
        core.position         = position;
        core.positionVariance = Vec3( 0.05f );
        core.velocity         = Vec3( 0.0f, 0.3f, 0.0f );
        core.velocityVariance = Vec3( 0.2f, 0.2f, 0.2f );
        core.gravity          = Vec3( 0.0f, 0.0f, 0.0f );
        core.drag             = 1.0f;
        core.lifetime         = 0.35f;
        core.lifetimeVariance = 0.1f;
        core.startSize        = 0.25f;
        core.endSize          = 0.05f;
        core.startColor       = Color( 0.2f, 0.85f, 0.1f, 0.9f );
        core.endColor         = Color( 0.1f, 0.6f, 0.05f, 0.0f );
        core.texture          = txtBubble1;
        core.count            = 2;
        ps.Emit( core );

        // Smaller trailing bubbles
        ParticleParms bubbles = {};
        bubbles.position         = position;
        bubbles.positionVariance = Vec3( 0.1f );
        bubbles.velocity         = Vec3( 0.0f, 0.5f, 0.0f );
        bubbles.velocityVariance = Vec3( 0.4f, 0.3f, 0.4f );
        bubbles.gravity          = Vec3( 0.0f, -1.0f, 0.0f );
        bubbles.drag             = 2.0f;
        bubbles.lifetime         = 0.25f;
        bubbles.lifetimeVariance = 0.1f;
        bubbles.startSize        = 0.12f;
        bubbles.endSize          = 0.02f;
        bubbles.startColor       = Color( 0.3f, 0.9f, 0.2f, 0.7f );
        bubbles.endColor         = Color( 0.15f, 0.7f, 0.1f, 0.0f );
        bubbles.texture          = txtBubble2;
        bubbles.count            = 1;
        ps.Emit( bubbles );

        // Poison fumes trail
        ParticleParms fumes = {};
        fumes.position         = position;
        fumes.positionVariance = Vec3( 0.08f );
        fumes.velocity         = Vec3( 0.0f, 0.2f, 0.0f );
        fumes.velocityVariance = Vec3( 0.3f, 0.15f, 0.3f );
        fumes.gravity          = Vec3( 0.0f, 0.3f, 0.0f );
        fumes.drag             = 3.0f;
        fumes.lifetime         = 0.5f;
        fumes.lifetimeVariance = 0.15f;
        fumes.startSize        = 0.2f;
        fumes.endSize          = 0.35f;
        fumes.startColor       = Color( 0.15f, 0.5f, 0.05f, 0.5f );
        fumes.endColor         = Color( 0.1f, 0.3f, 0.0f, 0.0f );
        fumes.texture               = txtFumes;
        fumes.rotationVariance      = glm::pi<f32>();  // random starting angle
        fumes.rotationSpeed         = 1.5f;
        fumes.rotationSpeedVariance = 1.0f;
        fumes.count                 = 1;
        ps.Emit( fumes );
    }

    void Entity_SpitterProjectile::EmitImpactBurst() {
        ParticleSystem & ps = map->GetParticleSystem();

        // Splash of poison bubbles
        ParticleParms splash = {};
        splash.position         = position;
        splash.positionVariance = Vec3( 0.1f );
        splash.velocity         = Vec3( 0.0f, 1.5f, 0.0f );
        splash.velocityVariance = Vec3( 2.5f, 2.0f, 2.5f );
        splash.gravity          = Vec3( 0.0f, -4.0f, 0.0f );
        splash.drag             = 1.5f;
        splash.lifetime         = 0.6f;
        splash.lifetimeVariance = 0.2f;
        splash.startSize        = 0.18f;
        splash.endSize          = 0.03f;
        splash.startColor       = Color( 0.2f, 0.9f, 0.15f, 1.0f );
        splash.endColor         = Color( 0.1f, 0.5f, 0.05f, 0.0f );
        splash.texture          = txtBubble1;
        splash.count            = 12;
        ps.Emit( splash );

        // Expanding poison cloud
        ParticleParms cloud = {};
        cloud.position         = position;
        cloud.positionVariance = Vec3( 0.15f );
        cloud.velocity         = Vec3( 0.0f, 0.5f, 0.0f );
        cloud.velocityVariance = Vec3( 1.0f, 0.5f, 1.0f );
        cloud.gravity          = Vec3( 0.0f, 0.2f, 0.0f );
        cloud.drag             = 3.0f;
        cloud.lifetime         = 0.8f;
        cloud.lifetimeVariance = 0.2f;
        cloud.startSize        = 0.3f;
        cloud.endSize          = 0.6f;
        cloud.startColor       = Color( 0.1f, 0.45f, 0.05f, 0.6f );
        cloud.endColor         = Color( 0.05f, 0.25f, 0.0f, 0.0f );
        cloud.texture               = txtFumes;
        cloud.rotationVariance      = glm::pi<f32>();
        cloud.rotationSpeed         = 1.0f;
        cloud.rotationSpeedVariance = 0.8f;
        cloud.count                 = 8;
        ps.Emit( cloud );
    }

    AlignedBox Entity_SpitterProjectile::GetBounds() const {
        const f32 h = RENDER_SCALE;
        return { position - Vec3( h ), position + Vec3( h ) };
    }

} // namespace atto
