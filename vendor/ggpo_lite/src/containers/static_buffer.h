/* -----------------------------------------------------------------------
 * GGPO.net (http://ggpo.net)  -  Copyright 2009 GroundStorm Studios, LLC.
 *
 * Use of this software is governed by the MIT license that can be found
 * in the LICENSE file.
 */

#ifndef _STATIC_BUFFER_H
#define _STATIC_BUFFER_H

#include "../types.h"

template<class _type_, int capacity> class StaticBuffer {
public:
    StaticBuffer<_type_, capacity>() :
        count( 0 ) {
    }

    _type_ & operator[]( int i ) {
        ASSERT( i >= 0 && i < count );
        return elements[ i ];
    }

    void PushBack( const _type_ & t ) {
        ASSERT( count != ( capacity - 1 ) );
        elements[ count++ ] = t;
    }

    int GetCount() {
        return count;
    }

protected:
    _type_        elements[ capacity ];
    int          count;
};

#endif
