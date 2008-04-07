// ==================================================================
// @(#)peer_t.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 23/05/2003
// @lastdate 02/05/2007
// ==================================================================

#ifndef __PEER_T_H__
#define __PEER_T_H__

#include <bgp/filter_t.h>
#include <bgp/rib_t.h>
#include <net/network.h>

typedef enum {
  SESSION_STATE_IDLE,
  SESSION_STATE_OPENWAIT,
  SESSION_STATE_ESTABLISHED,
  SESSION_STATE_ACTIVE,
  SESSION_STATE_MAX
} bgp_peer_state_t;

extern char * SESSION_STATES[SESSION_STATE_MAX];

// ----- Peer flags -----
/* The peer is a route-reflector client */
#define PEER_FLAG_RR_CLIENT     0x01

/* The next-hop of the routes received from this peer is set to the
   local-peer when the route is re-advertised */
#define PEER_FLAG_NEXT_HOP_SELF 0x02

/* The peer is virtual, i.e. there is no real BGP session with a peer
   router */
#define PEER_FLAG_VIRTUAL       0x04

/* This option prevents the Adj-RIB-in of the virtual peer to be
   cleared when the session is closed. Moreover, the routes already
   available in the Adj-RIB-in will be taken into account by the
   decision process when the session with the virtual peer is
   restarted. This option is only available to virtual peers. */
#define PEER_FLAG_SOFT_RESTART  0x08 

struct TBGPPeer;
typedef struct TBGPPeer SBGPPeer;
typedef struct TBGPPeer bgp_peer_t;

#endif /* __PEER_T_H__ */
