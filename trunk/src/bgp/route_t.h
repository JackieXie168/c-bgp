// ==================================================================
// @(#)route_t.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @auhtor Sebastien Tandel (standel@info.ucl.ac.be)
// 
// @date 20/11/2003
// @lastdate 10/04/2006
// ==================================================================

#ifndef __BGP_ROUTE_T_H__
#define __BGP_ROUTE_T_H__

#define __BGP_ROUTE_INFO_DP__

#include <libgds/array.h>
#include <libgds/types.h>

#include <bgp/comm_t.h>
#include <bgp/ecomm_t.h>
#include <bgp/peer_t.h>
#include <bgp/path.h>
#include <bgp/qos_t.h>
#include <bgp/route_reflector.h>
#include <bgp/types.h>
#include <net/prefix.h>
#include <net/network.h>

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

#ifdef __EXPERIMENTAL_ADVERTISE_BEST_EXTERNAL_TO_INTERNAL__
#define ROUTE_FLAG_EXTERNAL_BEST 0x0040
#endif

#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
#define ROUTE_FLAG_WALTON_BEST 0x0080 /* This flag is used to tell a neighbor
					 which was the best route selected by
					 the sender router. Then it is possible
					 to do the convergence as if it was the
					 normal BGP behavior without taking into
					 account the others routes received
					 from a specified neighbor.*/
#endif

/* QoS flags */
#define ROUTE_FLAG_BEST_EBGP  0x0100 /* best eBGP route */
#define ROUTE_FLAG_AGGR       0x0200 /* The route is a member of an
					aggregated route */

/* RR flags */
#define ROUTE_FLAG_RR_CLIENT  0x1000 /* The route was learned from an
					RR client */

#define ROUTE_PREF_DEFAULT 100
#define ROUTE_MED_MISSING  MAX_UINT32_T
#define ROUTE_MED_DEFAULT  0

#define ROUTE_SHOW_CISCO 0
#define ROUTE_SHOW_MRT   1

typedef struct TRoute {
  SPrefix sPrefix; // Destination prefix
  SPeer * pPeer;   // Peer of the route
  uint16_t uFlags; // Flags (best, feasible, eligible, ...)
#ifdef __BGP_ROUTE_INFO_DP__
  uint8_t tRank;   // How the route was selected
                   // (meaningful only if the route is currently best)
#endif
  SBGPAttr * pAttr;  // Route attributes

  /* Route-Reflection attributes */
  net_addr_t * pOriginator;
  SClusterList * pClusterList;

} SRoute;

#endif
