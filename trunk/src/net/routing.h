// ==================================================================
// @(#)routing.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 24/02/2004
// @lastdate 04/08/2005
// ==================================================================

#ifndef __NET_ROUTING_H__
#define __NET_ROUTING_H__

#include <stdio.h>

#include <libgds/array.h>
#include <libgds/types.h>

#include <net/prefix.h>
#include <net/link.h>
#include <net/routing_t.h>

#define NET_RT_SUCCESS               0
#define NET_RT_ERROR_NH_UNREACH     -1
#define NET_RT_ERROR_IF_UNKNOWN     -2
#define NET_RT_ERROR_ADD_DUP        -3
#define NET_RT_ERROR_DEL_UNEXISTING -4

// ----- SNetRouteInfoList ------------------------------------------
typedef SPtrArray SNetRouteInfoList;

// ----- SNetRouteNextHop -------------------------------------------
/**
 * This type defines a route next-hop. It is composed of the outgoing
 * link (iface) as well as the address of the destination node.
 *
 * If the outgoing link (iface) is a point-to-point link, the next-hop
 * address need not be specified. To the contrary, the next-hop
 * address is mandatory for a multi-point link such as a subnet.
 */
typedef struct {
  net_addr_t tAddr;
  SNetLink * pIface;
} SNetRouteNextHop;

// ----- SNetRouteInfo ----------------------------------------------
typedef struct {
  SPrefix sPrefix;
  uint32_t uWeight;
  SNetRouteNextHop sNextHop;
  net_route_type_t tType;
} SNetRouteInfo;

// ----- route_nexthop_compare --------------------------------------
extern int route_nexthop_compare(SNetRouteNextHop sNH1, SNetRouteNextHop sNH2);
// ----- rt_perror --------------------------------------------------
extern void rt_perror(FILE * pStream, int iErrorCode);
// ----- net_route_info_create --------------------------------------
extern SNetRouteInfo * net_route_info_create(SPrefix sPrefix,
					     SNetLink * pIface,
					     net_addr_t tNextHop,
					     uint32_t uWeight,
					     net_route_type_t tType);
// ----- net_route_info_destroy -------------------------------------
extern void net_route_info_destroy(SNetRouteInfo ** ppRouteInfo);

// ----- rt_create --------------------------------------------------
extern SNetRT * rt_create();
// ----- rt_destroy -------------------------------------------------
extern void rt_destroy(SNetRT ** ppRT);
// ----- rt_find_best -----------------------------------------------
extern SNetRouteInfo * rt_find_best(SNetRT * pRT, net_addr_t tAddr,
				    net_route_type_t tType);
// ----- rt_find_exact ----------------------------------------------
extern SNetRouteInfo * rt_find_exact(SNetRT * pRT, SPrefix sPrefix,
				     net_route_type_t tType);
// ----- rt_add_route -----------------------------------------------
extern int rt_add_route(SNetRT * pRT, SPrefix sPrefix,
			SNetRouteInfo * pRouteInfo);
// ----- rt_del_route -----------------------------------------------
extern int rt_del_route(SNetRT * pRT, SPrefix * pPrefix,
			SNetLink * pIface, net_addr_t * ptNextHop,
			net_route_type_t tType);
// ----- net_route_type_dump ----------------------------------------
extern void net_route_type_dump(FILE * pStream, net_route_type_t tType);
// ----- net_route_info_dump ----------------------------------------
extern void net_route_info_dump(FILE * pStream, SNetRouteInfo * pRouteInfo);
// ----- rt_info_list_dump ------------------------------------------
extern void rt_info_list_dump(FILE * pStream, SPrefix sPrefix,
			      SNetRouteInfoList * pRouteInfoList);
// ----- rt_dump ----------------------------------------------------
extern void rt_dump(FILE * pStream, SNetRT * pRT, SNetDest sDest);

// ----- rt_for_each ------------------------------------------------
extern int rt_for_each(SNetRT * pRT, FRadixTreeForEach fForEach,
		       void * pContext);

#endif /* __NET_ROUTING_H__ */
