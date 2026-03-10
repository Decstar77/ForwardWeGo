#include "atto_rng.h"



namespace atto {

    /*
    =============================
    =============================
    */
    RNG::RNG() {
    }

    /*
    =============================
    =============================
    */
    RNG::~RNG() {
    }

    /*
    =============================
    =============================
    */
    void RNG::Initialize( i32 seed ) {
        if ( seed == 0 ) {
            seed = std::random_device()();
        }
        rng.seed( seed );
        this->seed = seed;
    }

    /*
    =============================
    =============================
    */
    u32 RNG::Unsigned32( u32 min, u32 max ) {
        std::uniform_int_distribution<u32> dist( min, max );
        return dist( rng );
    }

    /*
    =============================
    =============================
    */
    u64 RNG::Unsigned64( u64 min, u64 max ) {
        std::uniform_int_distribution<u64> dist( min, max );
        return dist( rng );
    }

    /*
    =============================
    =============================
    */
    i32 RNG::Signed32( i32 min, i32 max ) {
        std::uniform_int_distribution<i32> dist( min, max );
        return dist( rng );
    }

    /*
    =============================
    =============================
    */
    i64 RNG::Signed64( i64 min, i64 max ) {
        std::uniform_int_distribution<i64> dist( min, max );
        return dist( rng );
    }

    /*
    =============================
    =============================
    */
    f32 RNG::Float( f32 min, f32 max ) {
        std::uniform_real_distribution<f32> dist( min, max );
        return dist( rng );
    }

    /*
    =============================
    =============================
    */
    f64 RNG::Double( f64 min, f64 max ) {
        std::uniform_real_distribution<f64> dist( min, max );
        return dist( rng );
    }

    /*
    =============================
    =============================
    */
    Vec2 RNG::Vec2f( f32 min, f32 max ) {
        return Vec2( Float( min, max ), Float( min, max ) );
    }
}