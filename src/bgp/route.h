// ==================================================================
// @(#)route.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 23/11/2002
// @lastdate 12/08/2004
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
			     origin_type_t uOriginType);
// ----- route_destroy ----------------------------------------------
extern void route_destroy(SRoute ** ppRoute);
// ----- route_flag_set ---------------------------------------------
extern void route_flag_set(SRoute * pRoute, uint16_t uFlag,
			   int iState);
// ----- route_flag_get ---------------------------------------------
extern int route_flag_get(SRoute * pRoute, uint16_t uFlag);
// ----- route_nexthop_set ------------------------------------------
extern void route_nexthop_set(SRoute * pRoute,
			      net_addr_t tNextHop);
// ----- route_nexthop_get ------------------------------------------
extern net_addr_t route_nexthop_get(SRoute * pRoute);
// ----- route_peer_set ---------------------------------------------
extern void route_peer_set(SRoute * pRoute, SPeer * pPeer);
// ----- route_peer_get ---------------------------------------------
extern SPeer * route_peer_get(SRoute * pRoute);
// ----- route_origin_set -------------------------------------------
extern void route_origin_set(SRoute * pRoute,
			     origin_type_t uOriginType);
// ----- route_origin_get -------------------------------------------
extern origin_type_t route_origin_get(SRoute * pRoute);
// ----- route_path_append ------------------------------------------
extern int route_path_append(SRoute * pRoute, uint16_t uAS);
// ----- route_path_prepend -----------------------------------------
extern int route_path_prepend(SRoute * pRoute, uint16_t uAS,
			      uint8_t uAmount);
// ----- route_path_contains ----------------------------------------
extern int route_path_contains(SRoute * pRoute, uint16_t uAS);
// ----- route_path_length ------------------------------------------
extern int route_path_length(SRoute * pRoute);
// ----- route_comm_append ------------------------------------------
extern int route_comm_append(SRoute * pRoute,
			     comm_t uCommunity);
// ----- route_comm_strip -------------------------------------------
extern void route_comm_strip(SRoute * pRoute);
// ----- route_comm_remove ------------------------------------------
extern void route_comm_remove(SRoute * pRoute, comm_t uCommunity);
// ----- route_comm_contains ----------------------------------------
extern int route_comm_contains(SRoute * pRoute,
			       comm_t uCommunity);
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
// ----- route_copy -------------------------------------------------
extern SRoute * route_copy(SRoute * pRoute);
// ----- route_dump -------------------------------------------------
extern void route_dump(FILE * pStream, SRoute * pRoute);
// ----- route_dump_mrtd --------------------------------------------
extern void route_dump_mrtd(FILE * pStream, SRoute * pRoute);
// ----- route_equals -----------------------------------------------
extern int route_equals(SRoute * pRoute1, SRoute * pRoute2);

#endif
