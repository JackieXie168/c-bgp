// ==================================================================
// @(#)routing.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 24/02/2004
// @lastdate 08/03/2004
// ==================================================================

#ifndef __NET_ROUTING_H__
#define __NET_ROUTING_H__

#include <stdio.h>

#include <libgds/array.h>
#include <libgds/radix-tree.h>
#include <libgds/types.h>

#include <net/prefix.h>
#include <net/link.h>

#define NET_ROUTE_STATIC 0
#define NET_ROUTE_IGP    1
#define NET_ROUTE_BGP    2

#define NET_RT_SUCCESS               0
#define NET_RT_ERROR_NH_UNREACH     -1
#define NET_RT_ERROR_IF_UNKNOWN     -2
#define NET_RT_ERROR_ADD_DUP        -3
#define NET_RT_ERROR_DEL_UNEXISTING -4

typedef uint8_t net_route_type_t;

typedef struct {
  uint32_t uWeight;
  SNetLink * pNextHopIf;
  net_route_type_t tType;
} SNetRouteInfo;

typedef SPtrArray SNetRouteInfoList;

typedef SRadixTree SNetRT;

// ----- rt_perror -------------------------------------------------------
extern void rt_perror(FILE * pStream, int iErrorCode);
// ----- route_info_create ------------------------------------------
extern SNetRouteInfo * route_info_create(SNetLink * pNextHopIf,
					 uint32_t uWeight,
					 net_route_type_t tType);
// ----- route_info_destroy -----------------------------------------
extern void route_info_destroy(SNetRouteInfo ** ppRouteInfo);

// ----- rt_create --------------------------------------------------
extern SNetRT * rt_create();
// ----- rt_destroy -------------------------------------------------
extern void rt_destroy(SNetRT ** ppRT);
// ----- rt_find_best -----------------------------------------------
extern SNetRouteInfo * rt_find_best(SNetRT * pRT, net_addr_t tAddr);
// ----- rt_find_exact ----------------------------------------------
extern SNetRouteInfo * rt_find_exact(SNetRT * pRT, SPrefix sPrefix);
// ----- rt_add_route -----------------------------------------------
extern int rt_add_route(SNetRT * pRT, SPrefix sPrefix,
			SNetRouteInfo * pRouteInfo);
// ----- rt_del_route -----------------------------------------------
extern int rt_del_route(SNetRT * pRT, SPrefix sPrefix,
			SNetLink * pNextHopIf, net_route_type_t tType);
// ----- rt_dump ----------------------------------------------------
extern void rt_dump(FILE * pStream, SNetRT * pRT, SPrefix sPrefix);

#endif
