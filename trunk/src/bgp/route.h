// ==================================================================
// @(#)route.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 23/11/2002
// @lastdate 21/07/2007
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
  SRoute * route_create(SPrefix sPrefix, SBGPPeer * pPeer,
			net_addr_t tNextHop, bgp_origin_t tOrigin);
  // ----- route_destroy --------------------------------------------
  void route_destroy(SRoute ** ppRoute);
  // ----- route_flag_set -------------------------------------------
  void route_flag_set(SRoute * pRoute, uint16_t uFlag, int iState);
  // ----- route_flag_get -------------------------------------------
  int route_flag_get(SRoute * pRoute, uint16_t uFlag);
  // ----- route_set_nexthop ----------------------------------------
  void route_set_nexthop(SRoute * pRoute, net_addr_t tNextHop);
  // ----- route_get_nexthop ----------------------------------------
  net_addr_t route_get_nexthop(SRoute * pRoute);
  
  // ----- route_peer_set -------------------------------------------
  void route_peer_set(SRoute * pRoute, SBGPPeer * pPeer);
  // ----- route_peer_get -------------------------------------------
  SBGPPeer * route_peer_get(SRoute * pRoute);
  
  // ----- route_set_origin -----------------------------------------
  void route_set_origin(SRoute * pRoute, bgp_origin_t tOrigin);
  // ----- route_get_origin -----------------------------------------
  bgp_origin_t route_get_origin(SRoute * pRoute);
  
  // -----[ route_get_path ]-----------------------------------------
  SBGPPath * route_get_path(SRoute * pRoute);
  // -----[ route_set_path ]-----------------------------------------
  void route_set_path(SRoute * pRoute, SBGPPath * pPath);
  // ----- route_path_prepend ---------------------------------------
  int route_path_prepend(SRoute * pRoute, uint16_t uAS, uint8_t uAmount);
  // -----[ route_path_rem_private ]---------------------------------
  int route_path_rem_private(SRoute * pRoute);
  // ----- route_path_contains --------------------------------------
  int route_path_contains(SRoute * pRoute, uint16_t uAS);
  // ----- route_path_length ----------------------------------------
  int route_path_length(SRoute * pRoute);
  // ----- route_path_last_as ---------------------------------------
  int route_path_last_as(SRoute * pRoute);
  
  // ----- route_set_comm -------------------------------------------
  void route_set_comm(SRoute * pRoute, SCommunities * pCommunities);
  // ----- route_comm_append ----------------------------------------
  int route_comm_append(SRoute * pRoute, comm_t tCommunity);
  // ----- route_comm_strip -----------------------------------------
  void route_comm_strip(SRoute * pRoute);
  // ----- route_comm_remove ----------------------------------------
  void route_comm_remove(SRoute * pRoute, comm_t tCommunity);
  // ----- route_comm_contains --------------------------------------
  int route_comm_contains(SRoute * pRoute, comm_t tCommunity);
  
  // ----- route_ecomm_append ---------------------------------------
  int route_ecomm_append(SRoute * pRoute, SECommunity * pComm);
  // ----- route_ecomm_strip_non_transitive -------------------------
  void route_ecomm_strip_non_transitive(SRoute * pRoute);
  
  // ----- route_localpref_set --------------------------------------
  void route_localpref_set(SRoute * pRoute, uint32_t uPref);
  // ----- route_localpref_get --------------------------------------
  uint32_t route_localpref_get(SRoute * pRoute);
  
  // ----- route_med_clear ------------------------------------------
  void route_med_clear(SRoute * pRoute);
  // ----- route_med_set --------------------------------------------
  void route_med_set(SRoute * pRoute, uint32_t uMED);
  // ----- route_med_get --------------------------------------------
  uint32_t route_med_get(SRoute * pRoute);
  
  // ----- route_originator_set -------------------------------------
  void route_originator_set(SRoute * pRoute, net_addr_t tOriginator);
  // ----- route_originator_get -------------------------------------
  int route_originator_get(SRoute * pRoute, net_addr_t * pOriginator);
  // ----- route_originator_clear -----------------------------------
  void route_originator_clear(SRoute * pRoute);
  
  // ----- route_cluster_list_set -----------------------------------
  void route_cluster_list_set(SRoute * pRoute);
  // ----- route_cluster_list_append --------------------------------
  void route_cluster_list_append(SRoute * pRoute, cluster_id_t tClusterID);
  // ----- route_cluster_list_clear ---------------------------------
  void route_cluster_list_clear(SRoute * pRoute);
  // ----- route_cluster_list_contains ------------------------------
  int route_cluster_list_contains(SRoute * pRoute, cluster_id_t tClusterID);
  
  // ----- route_router_list_append ---------------------------------
#ifdef __ROUTER_LIST_ENABLE__
  void route_router_list_append(SRoute * pRoute, net_addr_t tAddr);
#endif /* __ROUTER_LIST_ENABLE__ */

  // ----- route_copy -----------------------------------------------
  SRoute * route_copy(SRoute * pRoute);
  // ----- route_equals ---------------------------------------------
  int route_equals(SRoute * pRoute1, SRoute * pRoute2);


  ///////////////////////////////////////////////////////////////////
  // DUMP FUNCTIONS
  ///////////////////////////////////////////////////////////////////
  
  // -----[ route_str2format ]---------------------------------------
  int route_str2format(const char * pcFormat, uint8_t * puFormat);
  // ----- route_dump -----------------------------------------------
  void route_dump(SLogStream * pStream, SRoute * pRoute);
  // ----- route_dump_cisco -----------------------------------------
  void route_dump_cisco(SLogStream * pStream, SRoute * pRoute);
  // ----- route_dump_mrt -------------------------------------------
  void route_dump_mrt(SLogStream * pStream, SRoute * pRoute);
  // ----- route_dump_custom ----------------------------------------
  void route_dump_custom(SLogStream * pStream, SRoute * pRoute,
			 const char * pcFormat);
  
#ifdef __cplusplus
}
#endif

#endif /* __BGP_ROUTE_H__ */
