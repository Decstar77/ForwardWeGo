/* -----------------------------------------------------------------------
 * GGPO.net (http://ggpo.net)  -  Copyright 2009 GroundStorm Studios, LLC.
 *
 * Use of this software is governed by the MIT license that can be found
 * in the LICENSE file.
 */

#ifndef _GAMEINPUT_H
#define _GAMEINPUT_H

 // GAMEINPUT_MAX_BYTES * GAMEINPUT_MAX_PLAYERS * 8 must be less than
 // 2^BITVECTOR_NIBBLE_SIZE (see bitvector.h)

#define GAMEINPUT_MAX_BYTES      9
#define GAMEINPUT_MAX_PLAYERS    2

struct GameInput {
    enum Constants {
        NullFrame = -1
    };

    bool      IsNull();
    void      Initialize( int frame, char * bits, int size, int offset );
    void      Initialize( int frame, char * bits, int size );
    bool      Value( int i ) const;
    void      Set( int i );
    void      Clear( int i );
    void      Erase();
    void      Desc( char * buf, size_t buf_size, bool show_frame = true ) const;
    bool      Equal( GameInput & input, bool bitsonly = false );

    int      frame;
    int      size; /* size in bytes of the entire input for all players */
    char     bits[ GAMEINPUT_MAX_BYTES * GAMEINPUT_MAX_PLAYERS ];
};

#endif
