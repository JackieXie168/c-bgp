// ==================================================================
// @(#)route_t.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 20/11/2003
// @lastdate 03/01/2005
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
#define ROUTE_FLAG_FEASIBLE   0x0001 /* The route is feasible,
					i.e. the next-hop is reachable
					and the route has passed the
					input filters */
#define ROUTE_FLAG_ELIGIBLE   0x0002 /* The route is eligible, i.e. it
					has passed the input filters
				      */
#define ROUTE_FLAG_BEST       0x0004 /* The route is in the Loc-RIB
					and is installed in the
					routing table */
#define ROUTE_FLAG_ADVERTISED 0x0008 /* The route has been advertised
				      */
#define ROUTE_FLAG_DP_IGP     0x0010 /* The route has been chosen
					based on the IGP cost to the
					next-hop */
#define ROUTE_FLAG_INTERNAL   0x0020 /* The route is internal
					(i.e. added with the 'add
					network' statement */

/* QoS flags */
#define ROUTE_FLAG_BEST_EBGP  0x0100 /* best eBGP route */
#define ROUTE_FLAG_AGGR       0x0200 /* The route is a member of an
					aggregated route */

/* RR flags */
#define ROUTE_FLAG_RR_CLIENT  0x1000 /* The route was learned from an
					RR client */

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

#define ROUTE_PREF_DEFAULT 100
#define ROUTE_MED_MISSING  MAX_UINT32_T
#define ROUTE_MED_DEFAULT  0

#define ROUTE_SHOW_CISCO 0
#define ROUTE_SHOW_MRT   1

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
  uint16_t uFlags;

  // QoS information
#ifdef BGP_QOS
  qos_delay_t tDelay;
  qos_bandwidth_t tBandwidth;
  SPtrArray * pAggrRoutes;
  struct TRoute * pAggrRoute;
  struct TRoute * pEBGPRoute;
#endif

  // Route-Reflection
  net_addr_t * pOriginator;
  SClusterList * pClusterList;

} SRoute;

#endif

