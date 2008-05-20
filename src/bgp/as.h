// ==================================================================
// @(#)as.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @author Sebastien Tandel (standel@info.ucl.ac.be)
// @date 22/11/2002
// $Id: as.h,v 1.34 2008-05-20 11:58:15 bqu Exp $
// ==================================================================

#ifndef __BGP_ROUTER_H__
#define __BGP_ROUTER_H__

#include <stdio.h>

#include <libgds/array.h>
#include <libgds/types.h>
#include <libgds/list.h>

#include <bgp/as_t.h>
#include <bgp/dp_rules.h>
#include <bgp/filter.h>
#include <bgp/message.h>
#include <bgp/peer_t.h>
#include <bgp/rib.h>
#include <bgp/route.h>
#include <bgp/route-input.h>
#include <bgp/routes_list.h>
#include <net/prefix.h>
#include <net/message.h>
#include <net/network.h>

// ----- BGP Router Load RIB Options -----
#define BGP_ROUTER_LOAD_OPTIONS_SUMMARY  0x01  /* Display a summary (stderr) */
#define BGP_ROUTER_LOAD_OPTIONS_FORCE    0x02  /* Force the route to load */
#define BGP_ROUTER_LOAD_OPTIONS_AUTOCONF 0x04  /* Create non-existing peers */

extern FTieBreakFunction BGP_OPTIONS_TIE_BREAK;
extern uint32_t BGP_OPTIONS_DEFAULT_LOCAL_PREF;
extern uint8_t BGP_OPTIONS_MED_TYPE;
extern uint8_t BGP_OPTIONS_SHOW_MODE;
extern char * BGP_OPTIONS_SHOW_FORMAT;
extern uint8_t BGP_OPTIONS_AUTO_CREATE;

#ifdef __EXPERIMENTAL_ADVERTISE_BEST_EXTERNAL_TO_INTERNAL__
extern uint8_t BGP_OPTIONS_ADVERTISE_EXTERNAL_BEST;
#endif

#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
uint8_t BGP_OPTIONS_WALTON_CONVERGENCE_ON_BEST;
#endif

#define MAX_AS 65536

#define AS_LOG_DEBUG(AS, M) \
  LOG_DEBUG("AS%u:", AS->uNumber); \
  LOG_ENABLED_DEBUG(); \
  ip_address_dump(log_get_stream(pMainLog), AS->pNode->tAddr); \
  LOG_DEBUG(M);

// -----[ FBGPMsgListener ]-----
typedef void (*FBGPMsgListener)(net_msg_t * pMessage, void * pContext);

#ifdef __cplusplus
extern "C" {
#endif

  ///////////////////////////////////////////////////////////////////
  // BGP ROUTER MANAGEMENT FUNCTIONS
  ///////////////////////////////////////////////////////////////////

  // -----[ bgp_add_router ]-----------------------------------------
  net_error_t bgp_add_router(uint16_t uASN, net_node_t * pNode,
			     bgp_router_t ** ppRouter);

  // ----- bgp_router_create ----------------------------------------
  bgp_router_t * bgp_router_create(uint16_t uNumber, net_node_t * pNode);
  // ----- bgp_router_destroy ---------------------------------------
  void bgp_router_destroy(bgp_router_t ** ppRouter);
  // ----- bgp_router_set_name --------------------------------------
  //void bgp_router_set_name(bgp_router_t * pRouter, char * pcName);
  // ----- bgp_router_get_name --------------------------------------
  char * bgp_router_get_name(bgp_router_t * pRouter);
  // ----- bgp_router_add_peer --------------------------------------
  int bgp_router_add_peer(bgp_router_t * pRouter, uint16_t uRemoteAS,
			  net_addr_t tAddr, bgp_peer_t ** ppPeer);
  // ----- bgp_router_find_peer -------------------------------------
  bgp_peer_t * bgp_router_find_peer(bgp_router_t * pRouter, net_addr_t tAddr);
  // ----- bgp_router_peer_set_filter -------------------------------
  int bgp_router_peer_set_filter(bgp_router_t * pRouter, net_addr_t tAddr,
				 bgp_filter_dir_t dir,
				 bgp_filter_t * pFilter);
  // ----- bgp_router_add_network -----------------------------------
  int bgp_router_add_network(bgp_router_t * pRouter, SPrefix sPrefix);
  // ----- bgp_router_del_network -----------------------------------
  int bgp_router_del_network(bgp_router_t * pRouter, SPrefix sPrefix);
  // ----- as_add_qos_network ---------------------------------------
  int as_add_qos_network(bgp_router_t * pRouter, SPrefix sPrefix,
			 net_link_delay_t tDelay);
  // ----- bgp_router_start -----------------------------------------
  int bgp_router_start(bgp_router_t * pRouter);
  // ----- bgp_router_stop ------------------------------------------
  int bgp_router_stop(bgp_router_t * pRouter);
  // ----- bgp_router_reset -----------------------------------------
  int bgp_router_reset(bgp_router_t * pRouter);

  // ----- bgp_router_dump_adj_rib_in -------------------------------
  int bgp_router_dump_adj_rib_in(FILE * pStream, bgp_router_t * pRouter,
					SPrefix sPrefix);
  // ----- bgp_router_dump_rt_dp_rule -------------------------------
  int bgp_router_dump_rt_dp_rule(FILE * pStream, bgp_router_t * pRouter,
					SPrefix sPrefix);
  // ----- bgp_router_info ------------------------------------------
  void bgp_router_info(SLogStream * pStream, bgp_router_t * pRouter);
  // -----[ bgp_router_clear_adjrib ]--------------------------------
  int bgp_router_clear_adjrib(bgp_router_t * pRouter);
  
  // ----- bgp_router_decision_process_dop --------------------------
  /*void bgp_router_decision_process_dop(bgp_router_t * pRouter,
    SPtrArray * pRoutes);*/
  // ----- bgp_router_decision_process_disseminate_to_peer ----------
  void bgp_router_decision_process_disseminate_to_peer(bgp_router_t * pRouter,
						       SPrefix sPrefix,
						       bgp_route_t * pRoute,
						       bgp_peer_t * pPeer);
  // ----- bgp_router_decision_process_disseminate ------------------
  void bgp_router_decision_process_disseminate(bgp_router_t * pRouter,
					       SPrefix sPrefix,
					       bgp_route_t * pRoute);
  // ----- bgp_router_get_best_routes -------------------------------
  bgp_routes_t * bgp_router_get_best_routes(bgp_router_t * pRouter,
					    SPrefix sPrefix);
  // ----- bgp_router_get_feasible_routes ---------------------------
#ifdef __EXPERIMENTAL_ADVERTISE_BEST_EXTERNAL_TO_INTERNAL__
  bgp_routes_t * bgp_router_get_feasible_routes(bgp_router_t * pRouter,
						SPrefix sPrefix,
						uint8_t uEBGPRoute);
#else
  bgp_routes_t * bgp_router_get_feasible_routes(bgp_router_t * pRouter,
						SPrefix sPrefix);
#endif
  // ----- bgp_router_decision_process ------------------------------
  int bgp_router_decision_process(bgp_router_t * pRouter,
				  bgp_peer_t * pOriginPeer,
				  SPrefix sPrefix);
  // ----- bgp_router_handle_message --------------------------------
  int bgp_router_handle_message(simulator_t * sim,
				void * pRouter,
				net_msg_t * pMessage);
  // ----- bgp_router_ecomm_red_process -----------------------------
  int bgp_router_ecomm_red_process(bgp_peer_t * pPeer, bgp_route_t * pRoute);
  // ----- bgp_router_dump_id ---------------------------------------
  void bgp_router_dump_id(SLogStream * pStream, bgp_router_t * pRouter);
  
  // -----[ bgp_router_rerun ]---------------------------------------
  int bgp_router_rerun(bgp_router_t * pRouter, SPrefix sPrefix);
  // -----[ bgp_router_peer_readv_prefix ]---------------------------
  int bgp_router_peer_readv_prefix(bgp_router_t * pRouter,
				   bgp_peer_t * pPeer,
				   SPrefix sPrefix);
  
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  // ----- bgp_router_walton_peer_set -------------------------------
  int bgp_router_walton_peer_set(bgp_peer_t * pPeer, unsigned int uWaltonLimit);
#endif
  
  // ----- bgp_router_scan_rib --------------------------------------
  int bgp_router_scan_rib(bgp_router_t * pRouter);
  // ----- bgp_router_dump_networks ---------------------------------
  void bgp_router_dump_networks(SLogStream * pStream,
				       bgp_router_t * pRouter);
  // ----- bgp_router_networks_for_each -----------------------------
  int bgp_router_networks_for_each(bgp_router_t * pRouter,
				   FArrayForEach fForEach,
				   void * pContext);
  // ----- bgp_router_dump_peers ------------------------------------
  void bgp_router_dump_peers(SLogStream * pStream, bgp_router_t * pRouter);
  // ----- bgp_router_peers_for_each --------------------------------
  int bgp_router_peers_for_each(bgp_router_t * pRouter,
				FArrayForEach fForEach,
				void * pContext);
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  int bgp_router_peer_rib_out_remove(bgp_router_t * pRouter,
				     bgp_peer_t * pPeer,
				     SPrefix sPrefix,
				     net_addr_t * tNextHop);
#else
  int bgp_router_peer_rib_out_remove(bgp_router_t * pRouter,
				     bgp_peer_t * pPeer,
				     SPrefix sPrefix);
#endif
  // ----- bgp_router_dump_rib --------------------------------------
  void bgp_router_dump_rib(SLogStream * pStream, bgp_router_t * pRouter);
  // ----- bgp_router_dump_rib_address ------------------------------
  void bgp_router_dump_rib_address(SLogStream * pStream,
				   bgp_router_t * pRouter,
				   net_addr_t tAddr);
  // ----- bgp_router_dump_rib_prefix -------------------------------
  void bgp_router_dump_rib_prefix(SLogStream * pStream,
					 bgp_router_t * pRouter,
					 SPrefix sPrefix);
  // ----- bgp_router_dump_adjrib -----------------------------------
  void bgp_router_dump_adjrib(SLogStream * pStream,
			      bgp_router_t * pRouter,
			      bgp_peer_t * pPeer, SPrefix sPrefix,
			      bgp_rib_dir_t dir);

  ///////////////////////////////////////////////////////////////////
  // LOAD/SAVE FUNCTIONS
  ///////////////////////////////////////////////////////////////////

  // ----- bgp_router_load_rib --------------------------------------
  int bgp_router_load_rib(bgp_router_t * router, const char * filename,
			  bgp_input_type_t format, uint8_t options);
#ifdef __EXPERIMENTAL__
  // ----- bgp_router_load_ribs_in ----------------------------------
  int bgp_router_load_ribs_in(bgp_router_t * pRoutes, const char * pcFileName);
#endif
  // ----- bgp_router_save_rib --------------------------------------
  int bgp_router_save_rib(bgp_router_t * pRouter, const char * pcFileName);


  ///////////////////////////////////////////////////////////////////
  // MISCELLANEOUS FUNCTIONS
  ///////////////////////////////////////////////////////////////////

  // -----[ bgp_router_show_stats ]----------------------------------
  void bgp_router_show_stats(SLogStream * pStream, bgp_router_t * pRouter);
  // -----[ bgp_router_show_routes_info ]----------------------------
  int bgp_router_show_routes_info(SLogStream * pStream,
				  bgp_router_t * pRouter,
				  SNetDest sDest);
  // -----[ bgp_router_set_msg_listener ]----------------------------
  void bgp_router_set_msg_listener(FBGPMsgListener f, void * p);



  ///////////////////////////////////////////////////////////////////
  // FINALIZATION FUNCTIONS
  ///////////////////////////////////////////////////////////////////

  // -----[ _bgp_router_destroy ]------------------------------------
  void _bgp_router_destroy();

#ifdef __cplusplus
}
#endif

#endif /* __BGP_ROUTER_H__ */
