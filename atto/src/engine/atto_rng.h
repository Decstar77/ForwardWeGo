#pragma once

#include "atto_core.h"
#include "atto_math.h"

#include <random>

namespace atto {
    class RNG {
    public:
        RNG();
        ~RNG();

        void    Initialize( i32 seed = 0 );
        u32     Unsigned32( u32 min = 0, u32 max = U32_MAX );
        u64     Unsigned64( u64 min = 0, u64 max = U64_MAX );
        i32     Signed32( i32 min = I32_MIN, i32 max = I32_MAX );
        i64     Signed64( i64 min = I64_MIN, i64 max = I64_MAX );
        f32     Float( f32 min = 0.0f, f32 max = 1.0f );
        f64     Double( f64 min = 0.0, f64 max = 1.0 );
        Vec2    Vec2f( f32 min = 0.0f, f32 max = 1.0f );

    private:
        std::mt19937 rng;
        i32 seed;
    };
}