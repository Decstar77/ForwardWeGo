/* -----------------------------------------------------------------------
 * GGPO.net (http://ggpo.net)  -  Copyright 2009 GroundStorm Studios, LLC.
 *
 * Use of this software is governed by the MIT license that can be found
 * in the LICENSE file.
 */

#ifndef _SYNCTEST_H
#define _SYNCTEST_H

#include "backend.h"
#include "../types.h"
#include "../sync.h"
#include "../containers/ring_buffer.h"

class SyncTestBackend : public GGPOSession {
public:
    SyncTestBackend( GGPOSessionCallbacks * cb, char * gamename, int frames, int num_players );
    virtual ~SyncTestBackend();

    virtual GGPOErrorCode DoPoll( GGPOMessage * msgs, int num ) override;
    virtual GGPOErrorCode AddPlayer( GGPOPlayer * player, GGPOPlayerHandle * handle ) override;
    virtual GGPOErrorCode AddLocalInput( GGPOPlayerHandle player, void * values, int size ) override;
    virtual GGPOErrorCode SyncInput( void * values, int size, int * disconnect_flags ) override;
    virtual GGPOErrorCode IncrementFrame( void ) override;

protected:
    struct SavedInfo {
        int         frame;
        int         checksum;
        char *      buf;
        int         cbuf;
        GameInput   input;
    };

    void RaiseSyncError( const char * fmt, ... );
    void LogSaveStates( SavedInfo & info );

protected:
    GGPOSessionCallbacks   _callbacks;
    Sync                   _sync;
    int                    _num_players;
    int                    _check_distance;
    int                    _last_verified;
    bool                   _rollingback;
    bool                   _running;
    char                   _game[ 128 ];

    GameInput                  _current_input;
    GameInput                  _last_input;
    RingBuffer<SavedInfo, 32>  _saved_frames;
};

#endif

