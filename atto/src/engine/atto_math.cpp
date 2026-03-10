#include "atto_math.h"

#include <fpm/math.hpp>

namespace atto {

    fp FpLength( const fp2 & v ) {
        fp lenSq = FpLengthSquared( v );
        if ( lenSq == fp( 0 ) ) {
            return fp( 0 );
        }
        return fpm::sqrt( lenSq );
    }

    fp FpDistance( const fp2 & a, const fp2 & b ) {
        return FpLength( b - a );
    }

    fp2 FpNormalize( const fp2 & v ) {
        fp len = FpLength( v );
        if ( len == fp( 0 ) ) {
            return fp2();
        }
        return v / len;
    }

}
