// ==================================================================
// @(#)route.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 23/11/2002
// @lastdate 12/03/2008
// ==================================================================

#ifndef __BGP_ROUTE_H__
#define __BGP_ROUTE_H__

#include <libgds/array.h>

#include <stdio.h>
#include <stdlib.h>

#include <bgp/route_reflector.h>
#include <bgp/route_t.h>

// ----- BGP Routes Output Formats -----
#define BGP_ROUTES_OUTPUT_CISCO     0
#define BGP_ROUTES_OUTPUT_MRT_ASCII 1
#define BGP_ROUTES_OUTPUT_CUSTOM    2
#define BGP_ROUTES_OUTPUT_DEFAULT   BGP_ROUTES_OUTPUT_CISCO

#ifdef __cplusplus
extern "C" {
#endif

  // ----- route_create ---------------------------------------------
  bgp_route_t * route_create(SPrefix sPrefix, bgp_peer_t * pPeer,
			     net_addr_t tNextHop, bgp_origin_t tOrigin);
  // ----- route_destroy --------------------------------------------
  void route_destroy(bgp_route_t ** ppRoute);
  // ----- route_flag_set -------------------------------------------
  void route_flag_set(bgp_route_t * pRoute, uint16_t uFlag, int iState);
  // ----- route_flag_get -------------------------------------------
  int route_flag_get(bgp_route_t * pRoute, uint16_t uFlag);
  // ----- route_set_nexthop ----------------------------------------
  void route_set_nexthop(bgp_route_t * pRoute, net_addr_t tNextHop);
  // ----- route_get_nexthop ----------------------------------------
  net_addr_t route_get_nexthop(bgp_route_t * pRoute);
  
  // ----- route_peer_set -------------------------------------------
  void route_peer_set(bgp_route_t * pRoute, bgp_peer_t * pPeer);
  // ----- route_peer_get -------------------------------------------
  bgp_peer_t * route_peer_get(bgp_route_t * pRoute);
  
  // ----- route_set_origin -----------------------------------------
  void route_set_origin(bgp_route_t * pRoute, bgp_origin_t tOrigin);
  // ----- route_get_origin -----------------------------------------
  bgp_origin_t route_get_origin(bgp_route_t * pRoute);
  
  // -----[ route_get_path ]-----------------------------------------
  SBGPPath * route_get_path(bgp_route_t * pRoute);
  // -----[ route_set_path ]-----------------------------------------
  void route_set_path(bgp_route_t * pRoute, SBGPPath * pPath);
  // ----- route_path_prepend ---------------------------------------
  int route_path_prepend(bgp_route_t * pRoute, uint16_t uAS, uint8_t uAmount);
  // -----[ route_path_rem_private ]---------------------------------
  int route_path_rem_private(bgp_route_t * pRoute);
  // ----- route_path_contains --------------------------------------
  int route_path_contains(bgp_route_t * pRoute, uint16_t uAS);
  // ----- route_path_length ----------------------------------------
  int route_path_length(bgp_route_t * pRoute);
  // ----- route_path_last_as ---------------------------------------
  int route_path_last_as(bgp_route_t * pRoute);
  
  // ----- route_set_comm -------------------------------------------
  void route_set_comm(bgp_route_t * pRoute, SCommunities * pCommunities);
  // ----- route_comm_append ----------------------------------------
  int route_comm_append(bgp_route_t * pRoute, comm_t tCommunity);
  // ----- route_comm_strip -----------------------------------------
  void route_comm_strip(bgp_route_t * pRoute);
  // ----- route_comm_remove ----------------------------------------
  void route_comm_remove(bgp_route_t * pRoute, comm_t tCommunity);
  // ----- route_comm_contains --------------------------------------
  int route_comm_contains(bgp_route_t * pRoute, comm_t tCommunity);
  
  // ----- route_ecomm_append ---------------------------------------
  int route_ecomm_append(bgp_route_t * pRoute, SECommunity * pComm);
  // ----- route_ecomm_strip_non_transitive -------------------------
  void route_ecomm_strip_non_transitive(bgp_route_t * pRoute);
  
  // ----- route_localpref_set --------------------------------------
  void route_localpref_set(bgp_route_t * pRoute, uint32_t uPref);
  // ----- route_localpref_get --------------------------------------
  uint32_t route_localpref_get(bgp_route_t * pRoute);
  
  // ----- route_med_clear ------------------------------------------
  void route_med_clear(bgp_route_t * pRoute);
  // ----- route_med_set --------------------------------------------
  void route_med_set(bgp_route_t * pRoute, uint32_t uMED);
  // ----- route_med_get --------------------------------------------
  uint32_t route_med_get(bgp_route_t * pRoute);
  
  // ----- route_originator_set -------------------------------------
  void route_originator_set(bgp_route_t * pRoute, net_addr_t tOriginator);
  // ----- route_originator_get -------------------------------------
  int route_originator_get(bgp_route_t * pRoute, net_addr_t * pOriginator);
  // ----- route_originator_clear -----------------------------------
  void route_originator_clear(bgp_route_t * pRoute);
  
  // ----- route_cluster_list_set -----------------------------------
  void route_cluster_list_set(bgp_route_t * pRoute);
  // ----- route_cluster_list_append --------------------------------
  void route_cluster_list_append(bgp_route_t * pRoute, cluster_id_t tClusterID);
  // ----- route_cluster_list_clear ---------------------------------
  void route_cluster_list_clear(bgp_route_t * pRoute);
  // ----- route_cluster_list_contains ------------------------------
  int route_cluster_list_contains(bgp_route_t * pRoute, cluster_id_t tClusterID);
  
  // ----- route_copy -----------------------------------------------
  bgp_route_t * route_copy(bgp_route_t * pRoute);
  // ----- route_equals ---------------------------------------------
  int route_equals(bgp_route_t * pRoute1, bgp_route_t * pRoute2);


  ///////////////////////////////////////////////////////////////////
  // DUMP FUNCTIONS
  ///////////////////////////////////////////////////////////////////
  
  // -----[ route_str2format ]---------------------------------------
  int route_str2format(const char * pcFormat, uint8_t * puFormat);
  // ----- route_dump -----------------------------------------------
  void route_dump(SLogStream * pStream, bgp_route_t * pRoute);
  // ----- route_dump_cisco -----------------------------------------
  void route_dump_cisco(SLogStream * pStream, bgp_route_t * pRoute);
  // ----- route_dump_mrt -------------------------------------------
  void route_dump_mrt(SLogStream * pStream, bgp_route_t * pRoute);
  // ----- route_dump_custom ----------------------------------------
  void route_dump_custom(SLogStream * pStream, bgp_route_t * pRoute,
			 const char * pcFormat);
  
#ifdef __cplusplus
}
#endif

#endif /* __BGP_ROUTE_H__ */
