/* -----------------------------------------------------------------------
 * GGPO.net (http://ggpo.net)  -  Copyright 2009 GroundStorm Studios, LLC.
 *
 * Use of this software is governed by the MIT license that can be found
 * in the LICENSE file.
 */

#include "sync.h"

Sync::Sync( UdpMsg::connect_status * connect_status ) :
    local_connect_status( connect_status ),
    input_queues( NULL )
{
    frameCount = 0;
    last_confirmed_frame = -1;
    max_prediction_frames = 0;
    memset( &savedstate, 0, sizeof( savedstate ) );
}

Sync::~Sync() {
    /*
     * Delete frames manually here rather than in a destructor of the SavedFrame
     * structure so we can efficently copy frames via weak references.
     */
    for( int i = 0; i < ARRAY_SIZE( savedstate.frames ); i++ ) {
        callbacks.free_buffer( savedstate.frames[ i ].buf );
    }
    delete[] input_queues;
    input_queues = NULL;
}

void Sync::Init( Sync::Config & inconfig ) {
    config = inconfig;
    callbacks = inconfig.callbacks;
    frameCount = 0;
    rollingback = false;

    max_prediction_frames = inconfig.num_prediction_frames;

    CreateQueues( inconfig );
}

void Sync::SetLastConfirmedFrame( int frame ) {
    last_confirmed_frame = frame;
    if( last_confirmed_frame > 0 ) {
        for( int i = 0; i < config.num_players; i++ ) {
            input_queues[ i ].DiscardConfirmedFrames( frame - 1 );
        }
    }
}

bool Sync::AddLocalInput( int queue, GameInput & input ) {
    int frames_behind = frameCount - last_confirmed_frame;
    if( frameCount >= max_prediction_frames && frames_behind >= max_prediction_frames ) {
        Log( "Rejecting input from emulator: reached prediction barrier.\n" );
        return false;
    }

    if( frameCount == 0 ) {
        SaveCurrentFrame();
    }

    Log( "Sending undelayed local frame %d to queue %d.\n", frameCount, queue );
    input.frame = frameCount;
    input_queues[ queue ].AddInput( input );

    return true;
}

void Sync::AddRemoteInput( int queue, GameInput & input ) {
    input_queues[ queue ].AddInput( input );
}

int Sync::GetConfirmedInputs( void * values, int size, int frame ) {
    int disconnect_flags = 0;
    char * output = (char *)values;

    ASSERT( size >= config.num_players * config.input_size );

    memset( output, 0, size );
    for( int i = 0; i < config.num_players; i++ ) {
        GameInput input;
        if( local_connect_status[ i ].disconnected && frame > local_connect_status[ i ].last_frame ) {
            disconnect_flags |= ( 1 << i );
            input.Erase();
        }
        else {
            input_queues[ i ].GetConfirmedInput( frame, &input );
        }
        memcpy( output + ( i * config.input_size ), input.bits, config.input_size );
    }
    return disconnect_flags;
}

int Sync::SynchronizeInputs( void * values, int size ) {
    int disconnect_flags = 0;
    char * output = (char *)values;

    ASSERT( size >= config.num_players * config.input_size );

    memset( output, 0, size );
    for( int i = 0; i < config.num_players; i++ ) {
        GameInput input;
        if( local_connect_status[ i ].disconnected && frameCount > local_connect_status[ i ].last_frame ) {
            disconnect_flags |= ( 1 << i );
            input.Erase();
        }
        else {
            input_queues[ i ].GetInput( frameCount, &input );
        }
        memcpy( output + ( i * config.input_size ), input.bits, config.input_size );
    }
    return disconnect_flags;
}

void Sync::CheckSimulation() {
    int seek_to;
    if( !CheckSimulationConsistency( &seek_to ) ) {
        AdjustSimulation( seek_to );
    }
}

void Sync::IncrementFrame( void ) {
    frameCount++;
    SaveCurrentFrame();
}

void Sync::AdjustSimulation( int seek_to ) {
    int framecount = frameCount;
    int count = frameCount - seek_to;

    Log( "Catching up\n" );
    rollingback = true;

    /*
     * Flush our input queue and load the last frame.
     */
    LoadFrame( seek_to );
    ASSERT( frameCount == seek_to );

    /*
     * Advance frame by frame (stuffing notifications back to
     * the master).
     */
    ResetPrediction( frameCount );
    for( int i = 0; i < count; i++ ) {
        callbacks.advance_frame( 0 );
    }
    ASSERT( frameCount == framecount );

    rollingback = false;

    Log( "---\n" );
}

void Sync::LoadFrame( int frame ) {
    // find the frame in question
    if( frame == frameCount ) {
        Log( "Skipping NOP.\n" );
        return;
    }

    // Move the head pointer back and load it up
    savedstate.head = FindSavedFrameIndex( frame );
    SavedFrame * state = savedstate.frames + savedstate.head;

    Log( "=== Loading frame info %d (size: %d  checksum: %08x).\n",
        state->frame, state->cbuf, state->checksum );

    ASSERT( state->buf && state->cbuf );
    callbacks.load_game_state( state->buf, state->cbuf );

    // Reset framecount and the head of the state ring-buffer to point in
    // advance of the current frame (as if we had just finished executing it).
    frameCount = state->frame;
    savedstate.head = ( savedstate.head + 1 ) % ARRAY_SIZE( savedstate.frames );
}

void Sync::SaveCurrentFrame() {
    /*
     * See StateCompress for the real save feature implemented by FinalBurn.
     * Write everything into the head, then advance the head pointer.
     */
    SavedFrame * state = savedstate.frames + savedstate.head;
    if( state->buf ) {
        callbacks.free_buffer( state->buf );
        state->buf = NULL;
    }
    state->frame = frameCount;
    callbacks.save_game_state( &state->buf, &state->cbuf, &state->checksum, state->frame );

    Log( "=== Saved frame info %d (size: %d  checksum: %08x).\n", state->frame, state->cbuf, state->checksum );
    savedstate.head = ( savedstate.head + 1 ) % ARRAY_SIZE( savedstate.frames );
}

Sync::SavedFrame & Sync::GetLastSavedFrame() {
    int i = savedstate.head - 1;
    if( i < 0 ) {
        i = ARRAY_SIZE( savedstate.frames ) - 1;
    }
    return savedstate.frames[ i ];
}


int Sync::FindSavedFrameIndex( int frame ) {
    int i, count = ARRAY_SIZE( savedstate.frames );
    for( i = 0; i < count; i++ ) {
        if( savedstate.frames[ i ].frame == frame ) {
            break;
        }
    }
    if( i == count ) {
        ASSERT( false );
    }
    return i;
}

bool Sync::CreateQueues( Config & inconfig ) {
    delete[] input_queues;
    input_queues = new InputQueue[ config.num_players ];

    for( int i = 0; i < config.num_players; i++ ) {
        input_queues[ i ].Initialize( i, config.input_size );
    }
    return true;
}

bool Sync::CheckSimulationConsistency( int * seekTo ) {
    int first_incorrect = GameInput::NullFrame;
    for( int i = 0; i < config.num_players; i++ ) {
        int incorrect = input_queues[ i ].GetFirstIncorrectFrame();
        Log( "considering incorrect frame %d reported by queue %d.\n", incorrect, i );

        if( incorrect != GameInput::NullFrame && ( first_incorrect == GameInput::NullFrame || incorrect < first_incorrect ) ) {
            first_incorrect = incorrect;
        }
    }

    if( first_incorrect == GameInput::NullFrame ) {
        Log( "prediction ok.  proceeding.\n" );
        return true;
    }
    *seekTo = first_incorrect;
    return false;
}

void Sync::SetFrameDelay( int queue, int delay ) {
    input_queues[ queue ].SetFrameDelay( delay );
}

void Sync::ResetPrediction( int frameNumber ) {
    for( int i = 0; i < config.num_players; i++ ) {
        input_queues[ i ].ResetPrediction( frameNumber );
    }
}

bool Sync::GetEvent( Event & e ) {
    if( event_queue.GetCount() ) {
        e = event_queue.GetFront();
        event_queue.Pop();
        return true;
    }
    return false;
}


