/* -----------------------------------------------------------------------
 * GGPO.net (http://ggpo.net)  -  Copyright 2009 GroundStorm Studios, LLC.
 *
 * Use of this software is governed by the MIT license that can be found
 * in the LICENSE file.
 */

#ifndef _SPECTATOR_H
#define _SPECTATOR_H

#include "backend.h"
#include "../types.h"
#include "../sync.h"
#include "../timesync.h"
#include "../network/udp_proto.h"

#define SPECTATOR_FRAME_BUFFER_SIZE    64

class SpectatorBackend : public GGPOSession {
public:
   SpectatorBackend(GGPOSessionCallbacks *cb, const char *gamename, uint16 localport, int num_players, int input_size, GGPOPeerHandle peer);
   virtual ~SpectatorBackend();


public:
   virtual GGPOErrorCode DoPoll( GGPOMessage * msgs, int num ) override;
   virtual GGPOErrorCode AddPlayer(GGPOPlayer *player, GGPOPlayerHandle *handle) override { return GGPO_ERRORCODE_UNSUPPORTED; }
   virtual GGPOErrorCode AddLocalInput(GGPOPlayerHandle player, void *values, int size) override { return GGPO_OK; }
   virtual GGPOErrorCode SyncInput(void *values, int size, int *disconnect_flags) override;
   virtual GGPOErrorCode IncrementFrame(void) override;
   virtual GGPOErrorCode DisconnectPlayer(GGPOPlayerHandle handle) override { return GGPO_ERRORCODE_UNSUPPORTED; }
   virtual GGPOErrorCode GetNetworkStats(GGPONetworkStats *stats, GGPOPlayerHandle handle) override { return GGPO_ERRORCODE_UNSUPPORTED; }
   virtual GGPOErrorCode SetFrameDelay(GGPOPlayerHandle player, int delay) override { return GGPO_ERRORCODE_UNSUPPORTED; }
   virtual GGPOErrorCode SetDisconnectTimeout(int timeout) override { return GGPO_ERRORCODE_UNSUPPORTED; }
   virtual GGPOErrorCode SetDisconnectNotifyStart(int timeout) override { return GGPO_ERRORCODE_UNSUPPORTED; }

public:
   void OnMsg(GGPOPeerHandle from, UdpMsg *msg, int len);

protected:
   void PollUdpProtocolEvents(void);

   void OnUdpProtocolEvent(UdpProtocol::Event &e);

protected:
   GGPOSessionCallbacks  _callbacks;
   UdpProtocol           _host;
   bool                  _synchronizing;
   int                   _input_size;
   int                   _num_players;
   int                   _next_input_to_send;
   GameInput             _inputs[SPECTATOR_FRAME_BUFFER_SIZE];
};

#endif
