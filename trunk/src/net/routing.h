// ==================================================================
// @(#)routing.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 24/02/2004
// @lastdate 27/02/2004
// ==================================================================

#ifndef __NET_ROUTING_H__
#define __NET_ROUTING_H__

#include <stdio.h>

#include <libgds/radix-tree.h>
#include <libgds/types.h>

#include <net/prefix.h>
#include <net/link.h>

#define NET_ROUTE_STATIC 0
#define NET_ROUTE_IGP    1

typedef struct {
  uint32_t uWeight;
  SNetLink * pNextHopIf;
  uint8_t uType;
} SNetRouteInfo;

typedef SRadixTree SNetRT;

// ----- route_info_create ------------------------------------------
extern SNetRouteInfo * route_info_create(SNetLink * pNextHopIf,
					 uint32_t uWeight,
					 uint8_t uType);
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
// ----- rt_dump ----------------------------------------------------
extern void rt_dump(FILE * pStream, SNetRT * pRT, SPrefix sPrefix);

#endif
