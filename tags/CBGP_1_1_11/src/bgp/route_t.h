// ==================================================================
// @(#)route_t.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 20/11/2003
// @lastdate 13/04/2004
// ==================================================================

#ifndef __BGP_ROUTE_T_H__
#define __BGP_ROUTE_T_H__

#include <libgds/array.h>
#include <libgds/types.h>

#include <bgp/comm_t.h>
#include <bgp/ecomm_t.h>
#include <bgp/peer_t.h>
#include <bgp/path.h>
#include <bgp/qos_t.h>
#include <bgp/route_reflector.h>
#include <net/prefix.h>
#include <net/network.h>

typedef unsigned char origin_type_t;
#define ROUTE_ORIGIN_IGP 0        // static route
#define ROUTE_ORIGIN_EGP 1        // route learned through BGP
#define ROUTE_ORIGIN_INCOMPLETE 2 // route learned through redistribution

extern unsigned long rt_create_count;
extern unsigned long rt_copy_count;
extern unsigned long rt_destroy_count;

// Route flags
#define ROUTE_FLAG_FEASIBLE   0x01
#define ROUTE_FLAG_BEST       0x02
#define ROUTE_FLAG_ADVERTISED 0x04
#define ROUTE_FLAG_INTERNAL   0x80
// QoS: best eBGP route
#define ROUTE_FLAG_BEST_EBGP  0x08  
// QoS: member of an aggregated route
#define ROUTE_FLAG_AGGR       0x10
// RR: route learned from an RR client
#define ROUTE_FLAG_RR_CLIENT  0x20

// Route DP-Rule: rule of the decision process which has eventually
// selected the route as best
#define ROUTE_DP_RULE_NONE      0
#define ROUTE_DP_RULE_PREF      1
#define ROUTE_DP_RULE_PATH      2
#define ROUTE_DP_RULE_MED       3
#define ROUTE_DP_RULE_ORIGIN    4
#define ROUTE_DP_RULE_NEXT_HOP  5
#define ROUTE_DP_RULE_TIE_BREAK 6
#define ROUTE_DP_RULE_QOS_DELAY 10
#define ROUTE_DP_RULE_UNKNOWN   255

typedef struct TRoute {
  SPrefix sPrefix;
  SPeer * pPeer;
  net_addr_t tNextHop;
  origin_type_t uOriginType;
  SPath * pASPath;
  SCommunities * pCommunities;
  uint32_t uLocalPref;
  uint32_t uMED;
  SECommunities * pECommunities;
  uint32_t uTBID;
  uint8_t uFlags;

  // QoS information
  qos_delay_t tDelay;
  qos_bandwidth_t tBandwidth;
  SPtrArray * pAggrRoutes;
  struct TRoute * pAggrRoute;
  struct TRoute * pEBGPRoute;

  // Route-Reflection
  net_addr_t * pOriginator;
  SClusterList * pClusterList;

} SRoute;

#endif

