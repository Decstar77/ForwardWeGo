/* -----------------------------------------------------------------------
 * GGPO.net (http://ggpo.net)  -  Copyright 2009 GroundStorm Studios, LLC.
 *
 * Use of this software is governed by the MIT license that can be found
 * in the LICENSE file.
 */

#ifndef _RING_BUFFER_H
#define _RING_BUFFER_H

#include "../types.h"

template<class _type_, int capacity> class RingBuffer {
public:
    RingBuffer<_type_, capacity>() :
        head( 0 ),
        tail( 0 ),
        count( 0 ) {
    }

    _type_ & GetFront() {
        ASSERT( count != capacity );
        return elements[ tail ];
    }

    _type_ & GetItem( int i ) {
        ASSERT( i < count );
        return elements[ ( tail + i ) % capacity ];
    }

    void Pop() {
        ASSERT( count != capacity );
        tail = ( tail + 1 ) % capacity;
        count--;
    }

    void Push( const _type_ & t ) {
        ASSERT( count != ( capacity - 1 ) );
        elements[ head ] = t;
        head = ( head + 1 ) % capacity;
        count++;
    }

    int GetCount() {
        return count;
    }

    bool Clear() {
        return count == 0;
    }

protected:
    _type_           elements[ capacity ];
    int              head;
    int              tail;
    int              count;
};

#endif
