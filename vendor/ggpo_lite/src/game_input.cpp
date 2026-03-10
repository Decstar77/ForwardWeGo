/* -----------------------------------------------------------------------
 * GGPO.net (http://ggpo.net)  -  Copyright 2009 GroundStorm Studios, LLC.
 *
 * Use of this software is governed by the MIT license that can be found
 * in the LICENSE file.
 */

#include <stdio.h>
#include <memory.h>

#include "types.h"
#include "game_input.h"
#include "log.h"

bool GameInput::IsNull()
{
    return frame == NullFrame;
}

void GameInput::Initialize( int iframe, char * ibits, int isize, int offset ) {
    ASSERT( isize );
    ASSERT( isize <= GAMEINPUT_MAX_BYTES );
    frame = iframe;
    size = isize;
    memset( bits, 0, sizeof( bits ) );
    if( ibits ) {
        memcpy( bits + ( offset * isize ), ibits, isize );
    }
}

void GameInput::Initialize( int iframe, char * ibits, int isize ) {
    ASSERT( isize );
    ASSERT( isize <= GAMEINPUT_MAX_BYTES * GAMEINPUT_MAX_PLAYERS );
    frame = iframe;
    size = isize;
    memset( bits, 0, sizeof( bits ) );
    if( ibits ) {
        memcpy( bits, ibits, isize );
    }
}

bool GameInput::Value( int i ) const {
    return ( bits[ i / 8 ] & ( 1 << ( i % 8 ) ) ) != 0;
}

void GameInput::Set( int i ) {
    bits[ i / 8 ] |= ( 1 << ( i % 8 ) );
}

void GameInput::Clear( int i ) {
    bits[ i / 8 ] &= ~( 1 << ( i % 8 ) );
}

void GameInput::Erase() {
    memset( bits, 0, sizeof( bits ) );
}

void GameInput::Desc( char * buf, size_t buf_size, bool show_frame ) const {
    ASSERT( size );
    size_t remaining = buf_size;
    if( show_frame ) {
        remaining -= sprintf_s( buf, buf_size, "(frame:%d size:%d ", frame, size );
    }
    else {
        remaining -= sprintf_s( buf, buf_size, "(size:%d ", size );
    }

    for( int i = 0; i < size * 8; i++ ) {
        char buf2[ 16 ];
        if( Value( i ) ) {
            int c = sprintf_s( buf2, ARRAY_SIZE( buf2 ), "%2d ", i );
            strncat_s( buf, remaining, buf2, ARRAY_SIZE( buf2 ) );
            remaining -= c;
        }
    }
    strncat_s( buf, remaining, ")", 1 );
}

bool GameInput::Equal( GameInput & other, bool bitsonly ) {
    if( !bitsonly && frame != other.frame ) {
        Log( "frames don't match: %d, %d\n", frame, other.frame );
    }
    if( size != other.size ) {
        Log( "sizes don't match: %d, %d\n", size, other.size );
    }
    if( memcmp( bits, other.bits, size ) ) {
        Log( "bits don't match\n" );
    }
    ASSERT( size && other.size );
    return ( bitsonly || frame == other.frame ) && size == other.size && memcmp( bits, other.bits, size ) == 0;
}

