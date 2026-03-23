#pragma once

#include "atto_core.h"
#include "atto_math.h"

namespace atto {

    class Renderer;
    class Texture;

    static constexpr i32 MAX_PARTICLES = 4096;

    struct ParticleParms {
        Vec3    position = Vec3( 0.0f );
        Vec3    positionVariance = Vec3( 0.0f );

        Vec3    velocity = Vec3( 0.0f, 1.0f, 0.0f );
        Vec3    velocityVariance = Vec3( 0.0f );

        Vec3    gravity = Vec3( 0.0f, -9.8f, 0.0f );
        f32     drag = 0.0f;

        f32     lifetime = 1.0f;
        f32     lifetimeVariance = 0.0f;

        f32     startSize = 0.5f;
        f32     endSize = 0.0f;

        Color   startColor = Color::White();
        Color   endColor = Color( 1.0f, 1.0f, 1.0f, 0.0f );

        const Texture * texture = nullptr;

        bool    velocityAligned = false;
        f32     stretchFactor = 1.0f;

        i32     count = 1;
    };

    class ParticleSystem {
    public:
        struct Particle {
            Vec3    position;
            Vec3    velocity;
            Vec3    gravity;
            f32     drag;
            f32     lifetime;
            f32     maxLifetime;
            f32     startSize;
            f32     endSize;
            Color   startColor;
            Color   endColor;
            const Texture * texture;
            bool    velocityAligned;
            f32     stretchFactor;
        };

        void Emit( const ParticleParms & parms );
        void Update( f32 dt );
        void Render( Renderer & renderer, const Vec3 & cameraPos, const Vec3 & cameraUp );

        i32 GetAliveCount() const { return aliveCount; }
        void Clear() { aliveCount = 0; }

    private:
        Particle particles[MAX_PARTICLES] = {};
        i32 aliveCount = 0;
    };
}
