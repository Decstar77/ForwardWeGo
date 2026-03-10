/* -----------------------------------------------------------------------
 * GGPO.net (http://ggpo.net)  -  Copyright 2009 GroundStorm Studios, LLC.
 *
 * Use of this software is governed by the MIT license that can be found
 * in the LICENSE file.
 */

#include "../types.h"
#include "bitvector.h"

void BitVectorSetBit( uint8 * vector, int * offset ) {
    vector[ ( *offset ) / 8 ] |= ( 1 << ( ( *offset ) % 8 ) );
    *offset += 1;
}

void BitVectorClearBit( uint8 * vector, int * offset ) {
    vector[ ( *offset ) / 8 ] &= ~( 1 << ( ( *offset ) % 8 ) );
    *offset += 1;
}

void BitVectorWriteNibblet( uint8 * vector, int nibble, int * offset ) {
    ASSERT( nibble < ( 1 << BITVECTOR_NIBBLE_SIZE ) );
    for( int i = 0; i < BITVECTOR_NIBBLE_SIZE; i++ ) {
        if( nibble & ( 1 << i ) ) {
            BitVectorSetBit( vector, offset );
        }
        else {
            BitVectorClearBit( vector, offset );
        }
    }
}

int BitVectorReadBit( uint8 * vector, int * offset ) {
    int retval = !!( vector[ ( *offset ) / 8 ] & ( 1 << ( ( *offset ) % 8 ) ) );
    *offset += 1;
    return retval;
}

int BitVectorReadNibblet( uint8 * vector, int * offset ) {
    int nibblet = 0;
    for( int i = 0; i < BITVECTOR_NIBBLE_SIZE; i++ ) {
        nibblet |= ( BitVectorReadBit( vector, offset ) << i );
    }
    return nibblet;
}

