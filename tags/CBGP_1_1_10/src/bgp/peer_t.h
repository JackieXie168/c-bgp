// ==================================================================
// @(#)peer_t.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 23/05/2003
// @lastdate 04/02/2004
// ==================================================================

#ifndef __PEER_T_H__
#define __PEER_T_H__

#include <bgp/filter_t.h>
#include <bgp/rib_t.h>
#include <net/network.h>

#define SESSION_STATE_IDLE        0
#define SESSION_STATE_OPENWAIT    1
#define SESSION_STATE_ESTABLISHED 2

extern char * SESSION_STATES[3];

#define PEER_TYPE_CUSTOMER 0
#define PEER_TYPE_PROVIDER 1
#define PEER_TYPE_PEER     2
#define PEER_TYPE_UNKNOWN  255

#define PEER_FLAG_RR_CLIENT     1
#define PEER_FLAG_NEXT_HOP_SELF 2

struct TPeer;
typedef struct TPeer SPeer;

#endif
