/* -----------------------------------------------------------------------
 * GGPO.net (http://ggpo.net)  -  Copyright 2009 GroundStorm Studios, LLC.
 *
 * Use of this software is governed by the MIT license that can be found
 * in the LICENSE file.
 */

#include "../types.h"
#include "udp.h"
#include <iostream>

//Udp::Udp() {
//}
//
//Udp::~Udp() {
//}
//
//void Udp::Init( GGPOSessionCallbacks * cb, Callbacks * calls ) {
//    callbacks = calls;
//    sessionCallbacks = *cb;
//}
//
//void Udp::SendTo( char * buffer, int len, int flags, GGPOPeerHandle dst, GGPOPeerHandle local ) {
//    sessionCallbacks.network_send_to( local, dst, buffer, len );
//}
//
//bool Udp::OnLoopPoll( GGPOMessage * msgs, int num ) {
//    for( int i = 0; i < num; i++ ) {
//        GGPOMessage msg = msgs[ i ];
//        callbacks->OnMsg( msg.peer, (UdpMsg*)msg.buffer, msg.len );
//    }
//    
//    return true;
//}

