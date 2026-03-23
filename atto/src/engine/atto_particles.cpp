#include "atto_particles.h"
#include "atto_renderer.h"

namespace atto {

    static f32 RandRange( f32 base, f32 variance ) {
        if ( variance <= 0.0f ) {
            return base;
        }
        return base + (RandomFloat() * 2.0f - 1.0f) * variance;
    }

    static Vec3 RandRange( const Vec3 & base, const Vec3 & variance ) {
        return Vec3(
            RandRange( base.x, variance.x ),
            RandRange( base.y, variance.y ),
            RandRange( base.z, variance.z )
        );
    }

    void ParticleSystem::Emit( const ParticleParms & parms ) {
        for ( i32 i = 0; i < parms.count; i++ ) {
            if ( aliveCount >= MAX_PARTICLES ) {
                return;
            }

            Particle & p = particles[aliveCount++];
            p.position = RandRange( parms.position, parms.positionVariance );
            p.velocity = RandRange( parms.velocity, parms.velocityVariance );
            p.gravity = parms.gravity;
            p.drag = parms.drag;
            p.maxLifetime = Max( RandRange( parms.lifetime, parms.lifetimeVariance ), 0.01f );
            p.lifetime = p.maxLifetime;
            p.startSize = parms.startSize;
            p.endSize = parms.endSize;
            p.startColor = parms.startColor;
            p.endColor = parms.endColor;
            p.texture = parms.texture;
        }
    }

    void ParticleSystem::Update( f32 dt ) {
        for ( i32 i = 0; i < aliveCount; ) {
            Particle & p = particles[i];
            p.lifetime -= dt;

            if ( p.lifetime <= 0.0f ) {
                // Swap with last alive particle
                particles[i] = particles[aliveCount - 1];
                aliveCount--;
                continue;
            }

            // Apply gravity
            p.velocity += p.gravity * dt;

            // Apply drag
            if ( p.drag > 0.0f ) {
                p.velocity *= Max( 1.0f - p.drag * dt, 0.0f );
            }

            // Integrate position
            p.position += p.velocity * dt;

            i++;
        }
    }

    void ParticleSystem::Render( Renderer & renderer, const Vec3 & cameraPos, const Vec3 & cameraUp ) {
        if ( aliveCount <= 0 ) {
            return;
        }

        renderer.RenderParticles( particles, aliveCount, cameraPos, cameraUp );
    }
}
