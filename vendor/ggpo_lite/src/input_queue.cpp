/* -----------------------------------------------------------------------
 * GGPO.net (http://ggpo.net)  -  Copyright 2009 GroundStorm Studios, LLC.
 *
 * Use of this software is governed by the MIT license that can be found
 * in the LICENSE file.
 */

#include "types.h"
#include "input_queue.h"

#define PREVIOUS_FRAME(offset)   (((offset) == 0) ? (INPUT_QUEUE_LENGTH - 1) : ((offset) - 1))

InputQueue::InputQueue( int input_size ) {
    Initialize( -1, input_size );
}

InputQueue::~InputQueue() {
}

void InputQueue::Initialize( int _id, int input_size ) {
    id = _id;
    head = 0;
    tail = 0;
    length = 0;
    frame_delay = 0;
    first_frame = true;
    last_user_added_frame = GameInput::NullFrame;
    first_incorrect_frame = GameInput::NullFrame;
    last_frame_requested = GameInput::NullFrame;
    last_added_frame = GameInput::NullFrame;

    prediction.Initialize( GameInput::NullFrame, NULL, input_size );

    /*
     * This is safe because we know the GameInput is a proper structure (as in,
     * no virtual methods, no contained classes, etc.).
     */
    memset( inputs, 0, sizeof inputs );
    for( int i = 0; i < ARRAY_SIZE( inputs ); i++ ) {
        inputs[ i ].size = input_size;
    }
}

int InputQueue::GetFirstIncorrectFrame() {
    return first_incorrect_frame;
}

void InputQueue::DiscardConfirmedFrames( int frame ) {
    ASSERT( frame >= 0 );

    if( last_frame_requested != GameInput::NullFrame ) {
        frame = MIN( frame, last_frame_requested );
    }

    Log( "discarding confirmed frames up to %d (last_added:%d length:%d [head:%d tail:%d]).\n",
        frame, last_added_frame, length, head, tail );
    if( frame >= last_added_frame ) {
        tail = head;
    }
    else {
        int offset = frame - inputs[ tail ].frame + 1;

        Log( "difference of %d frames.\n", offset );
        ASSERT( offset >= 0 );

        tail = ( tail + offset ) % INPUT_QUEUE_LENGTH;
        length -= offset;
    }

    Log( "after discarding, new tail is %d (frame:%d).\n", tail, inputs[ tail ].frame );
    ASSERT( length >= 0 );
}

void InputQueue::ResetPrediction( int frame ) {
    ASSERT( first_incorrect_frame == GameInput::NullFrame || frame <= first_incorrect_frame );

    Log( "resetting all prediction errors back to frame %d.\n", frame );

    /*
     * There's nothing really to do other than reset our prediction
     * state and the incorrect frame counter...
     */
    prediction.frame = GameInput::NullFrame;
    first_incorrect_frame = GameInput::NullFrame;
    last_frame_requested = GameInput::NullFrame;
}

bool InputQueue::GetConfirmedInput( int requested_frame, GameInput * input ) {
    ASSERT( first_incorrect_frame == GameInput::NullFrame || requested_frame < first_incorrect_frame );
    int offset = requested_frame % INPUT_QUEUE_LENGTH;
    if( inputs[ offset ].frame != requested_frame ) {
        return false;
    }
    *input = inputs[ offset ];
    return true;
}

bool InputQueue::GetInput( int requested_frame, GameInput * input ) {
    Log( "requesting input frame %d.\n", requested_frame );

    /*
     * No one should ever try to grab any input when we have a prediction
     * error.  Doing so means that we're just going further down the wrong
     * path.  ASSERT this to verify that it's true.
     */
    ASSERT( first_incorrect_frame == GameInput::NullFrame );

    /*
     * Remember the last requested frame number for later.  We'll need
     * this in AddInput() to drop out of prediction mode.
     */
    last_frame_requested = requested_frame;

    ASSERT( requested_frame >= inputs[ tail ].frame );

    if( prediction.frame == GameInput::NullFrame ) {
        /*
         * If the frame requested is in our range, fetch it out of the queue and
         * return it.
         */
        int offset = requested_frame - inputs[ tail ].frame;

        if( offset < length ) {
            offset = ( offset + tail ) % INPUT_QUEUE_LENGTH;
            ASSERT( inputs[ offset ].frame == requested_frame );
            *input = inputs[ offset ];
            Log( "returning confirmed frame number %d.\n", input->frame );
            return true;
        }

        /*
         * The requested frame isn't in the queue.  Bummer.  This means we need
         * to return a prediction frame.  Predict that the user will do the
         * same thing they did last time.
         */
        if( requested_frame == 0 ) {
            Log( "basing new prediction frame from nothing, you're client wants frame 0.\n" );
            prediction.Erase();
        }
        else if( last_added_frame == GameInput::NullFrame ) {
            Log( "basing new prediction frame from nothing, since we have no frames yet.\n" );
            prediction.Erase();
        }
        else {
            Log( "basing new prediction frame from previously added frame (queue entry:%d, frame:%d).\n",
                PREVIOUS_FRAME( head ), inputs[ PREVIOUS_FRAME( head ) ].frame );
            prediction = inputs[ PREVIOUS_FRAME( head ) ];
        }
        prediction.frame++;
    }

    ASSERT( prediction.frame >= 0 );

    /*
     * If we've made it this far, we must be predicting.  Go ahead and
     * forward the prediction frame contents.  Be sure to return the
     * frame number requested by the client, though.
     */
    *input = prediction;
    input->frame = requested_frame;
    Log( "returning prediction frame number %d (%d).\n", input->frame, prediction.frame );

    return false;
}

void InputQueue::AddInput( GameInput & input ) {
    int new_frame;

    Log( "adding input frame number %d to queue.\n", input.frame );

    /*
     * These next two lines simply verify that inputs are passed in
     * sequentially by the user, regardless of frame delay.
     */
    ASSERT( last_user_added_frame == GameInput::NullFrame ||
        input.frame == last_user_added_frame + 1 );
    last_user_added_frame = input.frame;

    /*
     * Move the queue head to the correct point in preparation to
     * input the frame into the queue.
     */
    new_frame = AdvanceQueueHead( input.frame );
    if( new_frame != GameInput::NullFrame ) {
        AddDelayedInputToQueue( input, new_frame );
    }

    /*
     * Update the frame number for the input.  This will also set the
     * frame to GameInput::NullFrame for frames that get dropped (by
     * design).
     */
    input.frame = new_frame;
}

void InputQueue::AddDelayedInputToQueue( GameInput & input, int frame_number ) {
    Log( "adding delayed input frame number %d to queue.\n", frame_number );

    ASSERT( input.size == prediction.size );

    ASSERT( last_added_frame == GameInput::NullFrame || frame_number == last_added_frame + 1 );

    ASSERT( frame_number == 0 || inputs[ PREVIOUS_FRAME( head ) ].frame == frame_number - 1 );

    /*
     * Add the frame to the back of the queue
     */
    inputs[ head ] = input;
    inputs[ head ].frame = frame_number;
    head = ( head + 1 ) % INPUT_QUEUE_LENGTH;
    length++;
    first_frame = false;

    last_added_frame = frame_number;

    if( prediction.frame != GameInput::NullFrame ) {
        ASSERT( frame_number == prediction.frame );

        /*
         * We've been predicting...  See if the inputs we've gotten match
         * what we've been predicting.  If so, don't worry about it.  If not,
         * remember the first input which was incorrect so we can report it
         * in GetFirstIncorrectFrame()
         */
        if( first_incorrect_frame == GameInput::NullFrame && !prediction.Equal( input, true ) ) {
            Log( "frame %d does not match prediction.  marking error.\n", frame_number );
            first_incorrect_frame = frame_number;
        }

        /*
         * If this input is the same frame as the last one requested and we
         * still haven't found any mis-predicted inputs, we can dump out
         * of predition mode entirely!  Otherwise, advance the prediction frame
         * count up.
         */
        if( prediction.frame == last_frame_requested && first_incorrect_frame == GameInput::NullFrame ) {
            Log( "prediction is correct!  dumping out of prediction mode.\n" );
            prediction.frame = GameInput::NullFrame;
        }
        else {
            prediction.frame++;
        }
    }
    ASSERT( length <= INPUT_QUEUE_LENGTH );
}

int InputQueue::AdvanceQueueHead( int frame ) {
    Log( "advancing queue head to frame %d.\n", frame );

    int expected_frame = first_frame ? 0 : inputs[ PREVIOUS_FRAME( head ) ].frame + 1;

    frame += frame_delay;

    if( expected_frame > frame ) {
        /*
         * This can occur when the frame delay has dropped since the last
         * time we shoved a frame into the system.  In this case, there's
         * no room on the queue.  Toss it.
         */
        Log( "Dropping input frame %d (expected next frame to be %d).\n",
            frame, expected_frame );
        return GameInput::NullFrame;
    }

    while( expected_frame < frame ) {
        /*
         * This can occur when the frame delay has been increased since the last
         * time we shoved a frame into the system.  We need to replicate the
         * last frame in the queue several times in order to fill the space
         * left.
         */
        Log( "Adding padding frame %d to account for change in frame delay.\n", expected_frame );
        GameInput & last_frame = inputs[ PREVIOUS_FRAME( head ) ];
        AddDelayedInputToQueue( last_frame, expected_frame );
        expected_frame++;
    }

    ASSERT( frame == 0 || frame == inputs[ PREVIOUS_FRAME( head ) ].frame + 1 );
    return frame;
}

