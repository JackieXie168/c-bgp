// ==================================================================
// @(#)route.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 23/11/2002
// @lastdate 16/08/2006
// ==================================================================

#ifndef __BGP_ROUTE_H__
#define __BGP_ROUTE_H__

#include <libgds/array.h>

#include <stdio.h>
#include <stdlib.h>

#include <bgp/route_reflector.h>
#include <bgp/route_t.h>

// ----- route_create -----------------------------------------------
extern SRoute * route_create(SPrefix sPrefix, SPeer * pPeer,
			     net_addr_t tNextHop,
			     bgp_origin_t tOrigin);
// ----- route_destroy ----------------------------------------------
extern void route_destroy(SRoute ** ppRoute);
// ----- route_flag_set ---------------------------------------------
extern void route_flag_set(SRoute * pRoute, uint16_t uFlag,
			   int iState);
// ----- route_flag_get ---------------------------------------------
extern int route_flag_get(SRoute * pRoute, uint16_t uFlag);

// ----- route_set_nexthop ------------------------------------------
extern void route_set_nexthop(SRoute * pRoute,
			      net_addr_t tNextHop);
// ----- route_get_nexthop ------------------------------------------
extern net_addr_t route_get_nexthop(SRoute * pRoute);

// ----- route_peer_set ---------------------------------------------
extern void route_peer_set(SRoute * pRoute, SPeer * pPeer);
// ----- route_peer_get ---------------------------------------------
extern SPeer * route_peer_get(SRoute * pRoute);

// ----- route_set_origin -------------------------------------------
extern void route_set_origin(SRoute * pRoute,
			     bgp_origin_t tOrigin);
// ----- route_get_origin -------------------------------------------
extern bgp_origin_t route_get_origin(SRoute * pRoute);

// -----[ route_get_path ]-------------------------------------------
extern SBGPPath * route_get_path(SRoute * pRoute);
// -----[ route_set_path ]-------------------------------------------
extern void route_set_path(SRoute * pRoute, SBGPPath * pPath);
// ----- route_path_prepend -----------------------------------------
extern int route_path_prepend(SRoute * pRoute, uint16_t uAS,
			      uint8_t uAmount);
// ----- route_path_contains ----------------------------------------
extern int route_path_contains(SRoute * pRoute, uint16_t uAS);
// ----- route_path_length ------------------------------------------
extern int route_path_length(SRoute * pRoute);
// ----- route_path_last_as -----------------------------------------
extern int route_path_last_as(SRoute * pRoute);

// ----- route_set_comm ---------------------------------------------
extern void route_set_comm(SRoute * pRoute, SCommunities * pCommunities);
// ----- route_comm_append ------------------------------------------
extern int route_comm_append(SRoute * pRoute, comm_t tCommunity);
// ----- route_comm_strip -------------------------------------------
extern void route_comm_strip(SRoute * pRoute);
// ----- route_comm_remove ------------------------------------------
extern void route_comm_remove(SRoute * pRoute, comm_t tCommunity);
// ----- route_comm_contains ----------------------------------------
extern int route_comm_contains(SRoute * pRoute, comm_t tCommunity);

// ----- route_ecomm_append -----------------------------------------
extern int route_ecomm_append(SRoute * pRoute,
			      SECommunity * pComm);
// ----- route_ecomm_strip_non_transitive ---------------------------
extern void route_ecomm_strip_non_transitive(SRoute * pRoute);

// ----- route_localpref_set ----------------------------------------
extern void route_localpref_set(SRoute * pRoute, uint32_t uPref);
// ----- route_localpref_get ----------------------------------------
extern uint32_t route_localpref_get(SRoute * pRoute);

// ----- route_med_clear --------------------------------------------
extern void route_med_clear(SRoute * pRoute);
// ----- route_med_set ----------------------------------------------
extern void route_med_set(SRoute * pRoute, uint32_t uMED);
// ----- route_med_get ----------------------------------------------
extern uint32_t route_med_get(SRoute * pRoute);

// ----- route_originator_set ---------------------------------------
extern void route_originator_set(SRoute * pRoute, net_addr_t tOriginator);
// ----- route_originator_get ---------------------------------------
extern int route_originator_get(SRoute * pRoute, net_addr_t * pOriginator);
// ----- route_originator_clear -------------------------------------
extern void route_originator_clear(SRoute * pRoute);

// ----- route_cluster_list_set -------------------------------------
extern void route_cluster_list_set(SRoute * pRoute);
// ----- route_cluster_list_append ----------------------------------
extern void route_cluster_list_append(SRoute * pRoute,
					 cluster_id_t tClusterID);
// ----- route_cluster_list_clear -----------------------------------
extern void route_cluster_list_clear(SRoute * pRoute);
// ----- route_cluster_list_contains --------------------------------
extern int route_cluster_list_contains(SRoute * pRoute,
				       cluster_id_t tClusterID);

// ----- route_router_list_append -----------------------------------
#ifdef __ROUTER_LIST_ENABLE__
extern void route_router_list_append(SRoute * pRoute, net_addr_t tAddr);
#endif /* __ROUTER_LIST_ENABLE__ */
// ----- route_copy -------------------------------------------------
extern SRoute * route_copy(SRoute * pRoute);
// ----- route_dump_string ------------------------------------------
extern char * route_dump_string(SRoute * pRoute);
// ----- route_dump -------------------------------------------------
extern void route_dump(SLogStream * pStream, SRoute * pRoute);
// ----- route_dump_cisco -------------------------------------------
extern void route_dump_cisco(SLogStream * pStream, SRoute * pRoute);
// ----- route_dump_mrt ---------------------------------------------
extern void route_dump_mrt(SLogStream * pStream, SRoute * pRoute);
// ----- route_dump_custom ------------------------------------------
extern void route_dump_custom(SLogStream * pStream, SRoute * pRoute);
// ----- route_equals -----------------------------------------------
extern int route_equals(SRoute * pRoute1, SRoute * pRoute2);

#endif
