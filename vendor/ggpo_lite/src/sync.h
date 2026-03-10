/* -----------------------------------------------------------------------
 * GGPO.net (http://ggpo.net)  -  Copyright 2009 GroundStorm Studios, LLC.
 *
 * Use of this software is governed by the MIT license that can be found
 * in the LICENSE file.
 */

#ifndef _SYNC_H
#define _SYNC_H

#include "types.h"
#include "ggponet.h"
#include "game_input.h"
#include "input_queue.h"
#include "containers/ring_buffer.h"
#include "network/udp_msg.h"

#define MAX_PREDICTION_FRAMES    8

class SyncTestBackend;

class Sync {
public:
    struct Config {
        GGPOSessionCallbacks    callbacks;
        int                     num_prediction_frames;
        int                     num_players;
        int                     input_size;
    };
    struct Event {
        enum {
            ConfirmedInput,
        } type;
        union {
            struct {
                GameInput   input;
            } confirmedInput;
        } u;
    };

public:
    Sync( UdpMsg::connect_status * connect_status );
    virtual ~Sync();

    void                     Init( Config & config );
    void                     SetLastConfirmedFrame( int frame );
    void                     SetFrameDelay( int queue, int delay );
    bool                     AddLocalInput( int queue, GameInput & input );
    void                     AddRemoteInput( int queue, GameInput & input );
    int                      GetConfirmedInputs( void * values, int size, int frame );
    int                      SynchronizeInputs( void * values, int size );

    void                     CheckSimulation();
    void                     AdjustSimulation( int seek_to );
    void                     IncrementFrame( void );

    int                      GetFrameCount() { return frameCount; }
    bool                     InRollback() { return rollingback; }

    bool                     GetEvent( Event & e );

protected:
    friend SyncTestBackend;

    struct SavedFrame {
        byte *   buf;
        int      cbuf;
        int      frame;
        int      checksum;
        SavedFrame() : buf( NULL ), cbuf( 0 ), frame( -1 ), checksum( 0 ) { }
    };
    struct SavedState {
        SavedFrame  frames[ MAX_PREDICTION_FRAMES + 2 ];
        int         head;
    };

    void                        LoadFrame( int frame );
    void                        SaveCurrentFrame();
    int                         FindSavedFrameIndex( int frame );
    SavedFrame &                GetLastSavedFrame();

    bool                        CreateQueues( Config & config );
    bool                        CheckSimulationConsistency( int * seekTo );
    void                        ResetPrediction( int frameNumber );

protected:
    GGPOSessionCallbacks        callbacks;
    SavedState                  savedstate;
    Config                      config;

    bool                        rollingback;
    int                         last_confirmed_frame;
    int                         frameCount;
    int                         max_prediction_frames;

    InputQueue *                input_queues;

    RingBuffer<Event, 32>       event_queue;
    UdpMsg::connect_status *    local_connect_status;
};

#endif

