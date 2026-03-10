/* -----------------------------------------------------------------------
 * GGPO.net (http://ggpo.net)  -  Copyright 2009 GroundStorm Studios, LLC.
 *
 * Use of this software is governed by the MIT license that can be found
 * in the LICENSE file.
 */

#ifndef _INPUT_QUEUE_H
#define _INPUT_QUEUE_H

#include "game_input.h"

#define INPUT_QUEUE_LENGTH    128
#define DEFAULT_INPUT_SIZE      4

class InputQueue {
public:
    InputQueue( int input_size = DEFAULT_INPUT_SIZE );
    ~InputQueue();

    void            Initialize( int id, int input_size );
    int             GetFirstIncorrectFrame();

    void            SetFrameDelay( int delay ) { frame_delay = delay; }
    void            ResetPrediction( int frame );
    void            DiscardConfirmedFrames( int frame );
    bool            GetConfirmedInput( int frame, GameInput * input );
    bool            GetInput( int frame, GameInput * input );
    void            AddInput( GameInput & input );

protected:
    int             AdvanceQueueHead( int frame );
    void            AddDelayedInputToQueue( GameInput & input, int i );

protected:
    int             id;
    int             head;
    int             tail;
    int             length;
    bool            first_frame;

    int             last_user_added_frame;
    int             last_added_frame;
    int             first_incorrect_frame;
    int             last_frame_requested;

    int             frame_delay;

    GameInput       inputs[ INPUT_QUEUE_LENGTH ];
    GameInput       prediction;
};

#endif



