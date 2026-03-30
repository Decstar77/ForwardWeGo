#include "game_entity_spitter_projectile.h"
#include "game/game_map.h"

namespace atto {

    ATTO_REGISTER_CLASS( Entity, Entity_SpitterProjectile, EntityType::SpitterProjectile )

    Entity_SpitterProjectile::Entity_SpitterProjectile() {
        type         = EntityType::SpitterProjectile;
        isCollidable = false;
    }

    void Entity_SpitterProjectile::OnSpawn() {
        model = Engine::Get().GetRenderer().GetOrLoadStaticModel( "assets/models/primitives/sphere.obj" );
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

        // Check player hit every frame
        const Vec3 playerPos = map->GetPlayerPosition();
        if ( glm::length( playerPos - position ) < HIT_RADIUS ) {
            map->DamagePlayer( DAMAGE );
            map->DestroyEntity( this );
            return;
        }

        // Destroy when arc completes
        if ( t >= 1.0f ) {
            map->DestroyEntity( this );
        }
    }

    void Entity_SpitterProjectile::OnRender( Renderer & renderer ) {
        if ( !model || !launched ) {
            return;
        }

        const Mat4 modelMat = glm::translate( Mat4( 1.0f ), position )
                            * glm::scale( Mat4( 1.0f ), Vec3( RENDER_SCALE ) );

        renderer.RenderStaticModel( model, modelMat, Vec3( 0.15f, 0.9f, 0.2f ) );
    }

    void Entity_SpitterProjectile::OnDespawn() {
    }

    AlignedBox Entity_SpitterProjectile::GetBounds() const {
        const f32 h = RENDER_SCALE;
        return { position - Vec3( h ), position + Vec3( h ) };
    }

} // namespace atto
