// ==================================================================
// @(#)as_t.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @author Sebastien Tandel (standel@info.ucl.ac.be)
// @date 23/05/2003
// $Id: as_t.h,v 1.11 2008-04-11 11:03:06 bqu Exp $
// ==================================================================

#ifndef __AS_T_H__
#define __AS_T_H__

#include <libgds/array.h>
#include <bgp/peer-list.h>
#include <bgp/rib_t.h>
#include <bgp/route_reflector.h>
#include <bgp/tie_breaks.h>
#include <net/network.h>
#include <bgp/types.h>

typedef uint16_t asn_t;

typedef struct {
  asn_t        asn;
  char       * name;
  SRadixTree * routers;
} bgp_domain_t;

typedef struct TBGPRouter {
  asn_t          uASN;           /* AS-number of the router */
  net_addr_t     tRouterID;      /* Router-ID of the router */
  bgp_peers_t  * pPeers;         /* List of neighbor routers */
  SRIB         * pLocRIB;
  bgp_routes_t * pLocalNetworks; /* List of locally originated prefixes */
  //FTieBreakFunction fTieBreak; DEPRECATED ?
  cluster_id_t   tClusterID;     /* Cluster-ID */
  int            iRouteReflector;
  net_node_t   * pNode;          /* Reference to the node running this
				    BGP router */
  bgp_domain_t * domain;         /* Reference to the AS containing this
				    router */

#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  SPtrArray * pWaltonLimitPeers; /* This is a list of neighbors sorted on 
				    the walton limit number
				    TODO: is it possible to get rid of the
				    SPtrArray * pPeers when using walton ? */
#endif

} bgp_router_t;

#endif /* __AS_T_H__ */
