/* -----------------------------------------------------------------------
 * GGPO.net (http://ggpo.net)  -  Copyright 2009 GroundStorm Studios, LLC.
 *
 * Use of this software is governed by the MIT license that can be found
 * in the LICENSE file.
 */

#ifndef _P2P_H
#define _P2P_H

#include "backend.h"
#include "../types.h"
#include "../sync.h"
#include "../timesync.h"
#include "../network/udp_proto.h"

class Peer2PeerBackend : public GGPOSession {
public:
    Peer2PeerBackend( GGPOSessionCallbacks * cb, const char * gamename, int num_players, int input_size, GGPOPeerHandle local );
    virtual ~Peer2PeerBackend();

public:
    virtual GGPOErrorCode   DoPoll( GGPOMessage * msgs, int num ) override;
    virtual GGPOErrorCode   AddPlayer( GGPOPlayer * player, GGPOPlayerHandle * handle ) override;
    virtual GGPOErrorCode   AddLocalInput( GGPOPlayerHandle player, void * values, int size ) override;
    virtual GGPOErrorCode   SyncInput( void * values, int size, int * disconnect_flags ) override;
    virtual GGPOErrorCode   IncrementFrame( void ) override;
    virtual GGPOErrorCode   DisconnectPlayer( GGPOPlayerHandle handle ) override;
    virtual GGPOErrorCode   GetNetworkStats( GGPONetworkStats * stats, GGPOPlayerHandle handle ) override;
    virtual GGPOErrorCode   SetFrameDelay( GGPOPlayerHandle player, int delay ) override;
    virtual GGPOErrorCode   SetDisconnectTimeout( int timeout ) override;
    virtual GGPOErrorCode   SetDisconnectNotifyStart( int timeout ) override;

protected:
    void                    OnMsg( GGPOPeerHandle from, UdpMsg * msg, int len );
    
    GGPOErrorCode           PlayerHandleToQueue( GGPOPlayerHandle player, int * queue );
    GGPOPlayerHandle        QueueToPlayerHandle( int queue ) { return (GGPOPlayerHandle)( queue + 1 ); }
    GGPOPlayerHandle        QueueToSpectatorHandle( int queue ) { return (GGPOPlayerHandle)( queue + 1000 ); } /* out of range of the player array, basically */
    void                    DisconnectPlayerQueue( int queue, int syncto );
    void                    PollSyncEvents( void );
    void                    PollUdpProtocolEvents( void );
    void                    CheckInitialSync( void );
    int                     Poll2Players( int current_frame );
    int                     PollNPlayers( int current_frame );
    void                    AddRemotePlayer( GGPOPeerHandle peer, int queue );
    GGPOErrorCode           AddSpectator( GGPOPeerHandle peer );
    virtual void            OnSyncEvent( Sync::Event & e ) { }
    virtual void            OnUdpProtocolEvent( UdpProtocol::Event & e, GGPOPlayerHandle handle );
    virtual void            OnUdpProtocolPeerEvent( UdpProtocol::Event & e, int queue );
    virtual void            OnUdpProtocolSpectatorEvent( UdpProtocol::Event & e, int queue );

protected:
    GGPOPeerHandle          local;

    GGPOSessionCallbacks    _callbacks;
    Sync                    _sync;

    int                     _num_players;
    UdpProtocol *           _endpoints;

    int                     _num_spectators;
    UdpProtocol             _spectators[ GGPO_MAX_SPECTATORS ];

    int                     _input_size;
    bool                    _synchronizing;
    int                     _next_recommended_sleep;

    int                     _next_spectator_frame;
    int                     _disconnect_timeout;
    int                     _disconnect_notify_start;

    UdpMsg::connect_status  _local_connect_status[ UDP_MSG_MAX_PLAYERS ];
};

#endif
