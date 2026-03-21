#pragma once

#include "atto_core.h"

namespace atto {

    class Renderer;

    struct ParticleParms {

    };

    class ParticleSystem {
    public:
        void PushParticle( ParticleParms parms );

        void Update( f32 dt );
        void Render( Renderer & renderer );

        // Add stuff here...

    private:

    
    };
}