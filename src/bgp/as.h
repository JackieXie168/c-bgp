// ==================================================================
// @(#)as.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @author Sebastien Tandel (standel@info.ucl.ac.be)
// @date 22/11/2002
// @lastdate 31/05/2007
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
#include <bgp/routes_list.h>
#include <net/prefix.h>
#include <net/message.h>
#include <net/network.h>

#define BGP_NLRI_BE        0
#define BGP_NLRI_QOS_DELAY 1

// ----- BGP Router Load RIB Options -----
#define BGP_ROUTER_LOAD_OPTIONS_SUMMARY 0x01
#define BGP_ROUTER_LOAD_OPTIONS_FORCE   0x02

extern unsigned long route_create_count;
extern unsigned long route_copy_count;
extern unsigned long route_destroy_count;

extern FTieBreakFunction BGP_OPTIONS_TIE_BREAK;
extern uint8_t BGP_OPTIONS_NLRI;
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

// -----[ decision process ]-----------------------------------------
#define DP_NUM_RULES 11
extern char * DP_RULE_NAME[DP_NUM_RULES];
extern FDPRule DP_RULES[DP_NUM_RULES];

// -----[ FBGPMsgListener ]-----
typedef void (*FBGPMsgListener)(SNetMessage * pMessage, void * pContext);

#ifdef __cplusplus
extern "C" {
#endif

  ///////////////////////////////////////////////////////////////////
  // BGP ROUTER MANAGEMENT FUNCTIONS
  ///////////////////////////////////////////////////////////////////

  // ----- bgp_router_create ----------------------------------------
  SBGPRouter * bgp_router_create(uint16_t uNumber, SNetNode * pNode);
  // ----- bgp_router_destroy ---------------------------------------
  void bgp_router_destroy(SBGPRouter ** ppRouter);
  // ----- bgp_router_set_name --------------------------------------
  //void bgp_router_set_name(SBGPRouter * pRouter, char * pcName);
  // ----- bgp_router_get_name --------------------------------------
  char * bgp_router_get_name(SBGPRouter * pRouter);
  // ----- bgp_router_add_peer --------------------------------------
  int bgp_router_add_peer(SBGPRouter * pRouter, uint16_t uRemoteAS,
			  net_addr_t tAddr, SBGPPeer ** ppPeer);
  // ----- bgp_router_find_peer -------------------------------------
  SBGPPeer * bgp_router_find_peer(SBGPRouter * pRouter, net_addr_t tAddr);
  // ----- bgp_router_peer_set_filter -------------------------------
  int bgp_router_peer_set_filter(SBGPRouter * pRouter, net_addr_t tAddr,
				 SFilter * pFilter, int iIn);
  // ----- bgp_router_add_network -----------------------------------
  int bgp_router_add_network(SBGPRouter * pRouter, SPrefix sPrefix);
  // ----- bgp_router_del_network -----------------------------------
  int bgp_router_del_network(SBGPRouter * pRouter, SPrefix sPrefix);
  // ----- as_add_qos_network ---------------------------------------
  int as_add_qos_network(SBGPRouter * pRouter, SPrefix sPrefix,
			 net_link_delay_t tDelay);
  // ----- bgp_router_start -----------------------------------------
  int bgp_router_start(SBGPRouter * pRouter);
  // ----- bgp_router_stop ------------------------------------------
  int bgp_router_stop(SBGPRouter * pRouter);
  // ----- bgp_router_reset -----------------------------------------
  int bgp_router_reset(SBGPRouter * pRouter);




  // ----- bgp_router_dump_adj_rib_in -------------------------------
  int bgp_router_dump_adj_rib_in(FILE * pStream, SBGPRouter * pRouter,
					SPrefix sPrefix);
  // ----- bgp_router_dump_rt_dp_rule -------------------------------
  int bgp_router_dump_rt_dp_rule(FILE * pStream, SBGPRouter * pRouter,
					SPrefix sPrefix);
  // ----- bgp_router_info ------------------------------------------
  void bgp_router_info(SLogStream * pStream, SBGPRouter * pRouter);
  // -----[ bgp_router_clear_adjrib ]--------------------------------
  int bgp_router_clear_adjrib(SBGPRouter * pRouter);
  
  // ----- bgp_router_decision_process_dop --------------------------
  /*void bgp_router_decision_process_dop(SBGPRouter * pRouter,
    SPtrArray * pRoutes);*/
  // ----- bgp_router_decision_process_disseminate_to_peer ----------
  void bgp_router_decision_process_disseminate_to_peer(SBGPRouter * pRouter,
						       SPrefix sPrefix,
						       SRoute * pRoute,
						       SBGPPeer * pPeer);
  // ----- bgp_router_decision_process_disseminate ------------------
  void bgp_router_decision_process_disseminate(SBGPRouter * pRouter,
					       SPrefix sPrefix,
					       SRoute * pRoute);
  // ----- bgp_router_get_best_routes -------------------------------
  SRoutes * bgp_router_get_best_routes(SBGPRouter * pRouter,
				       SPrefix sPrefix);
  // ----- bgp_router_get_feasible_routes ---------------------------
#ifdef __EXPERIMENTAL_ADVERTISE_BEST_EXTERNAL_TO_INTERNAL__
  SRoutes * bgp_router_get_feasible_routes(SBGPRouter * pRouter,
					   SPrefix sPrefix,
					   uint8_t uEBGPRoute);
#else
  SRoutes * bgp_router_get_feasible_routes(SBGPRouter * pRouter,
					   SPrefix sPrefix);
#endif
  // ----- bgp_router_decision_process ------------------------------
  int bgp_router_decision_process(SBGPRouter * pRouter,
				  SBGPPeer * pOriginPeer,
				  SPrefix sPrefix);
  // ----- bgp_router_handle_message --------------------------------
  int bgp_router_handle_message(SSimulator * pSimulator,
				void * pRouter,
				SNetMessage * pMessage);
  // ----- bgp_router_ecomm_red_process -----------------------------
  int bgp_router_ecomm_red_process(SBGPPeer * pPeer, SRoute * pRoute);
  // ----- bgp_router_dump_id ---------------------------------------
  void bgp_router_dump_id(SLogStream * pStream, SBGPRouter * pRouter);
  
  // -----[ bgp_router_rerun ]---------------------------------------
  int bgp_router_rerun(SBGPRouter * pRouter, SPrefix sPrefix);
  // -----[ bgp_router_peer_readv_prefix ]---------------------------
  int bgp_router_peer_readv_prefix(SBGPRouter * pRouter,
				   SBGPPeer * pPeer,
				   SPrefix sPrefix);
  
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  // ----- bgp_router_walton_peer_set -------------------------------
  int bgp_router_walton_peer_set(SBGPPeer * pPeer, unsigned int uWaltonLimit);
#endif
  
  // ----- bgp_router_scan_rib --------------------------------------
  int bgp_router_scan_rib(SBGPRouter * pRouter);
  // ----- bgp_router_dump_networks ---------------------------------
  void bgp_router_dump_networks(SLogStream * pStream,
				       SBGPRouter * pRouter);
  // ----- bgp_router_networks_for_each -----------------------------
  int bgp_router_networks_for_each(SBGPRouter * pRouter,
				   FArrayForEach fForEach,
				   void * pContext);
  // ----- bgp_router_dump_peers ------------------------------------
  void bgp_router_dump_peers(SLogStream * pStream, SBGPRouter * pRouter);
  // ----- bgp_router_peers_for_each --------------------------------
  int bgp_router_peers_for_each(SBGPRouter * pRouter,
				FArrayForEach fForEach,
				void * pContext);
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  int bgp_router_peer_rib_out_remove(SBGPRouter * pRouter,
				     SBGPPeer * pPeer,
				     SPrefix sPrefix,
				     net_addr_t * tNextHop);
#else
  int bgp_router_peer_rib_out_remove(SBGPRouter * pRouter,
				     SBGPPeer * pPeer,
				     SPrefix sPrefix);
#endif
  // ----- _bgp_router_peers_compare --------------------------------
  int _bgp_router_peers_compare(void * pItem1, void * pItem2,
				unsigned int uEltSize);
  // ----- bgp_router_dump_rib_string -------------------------------
  char * bgp_router_dump_rib_string(SBGPRouter * pRouter);
  // ----- bgp_router_dump_rib --------------------------------------
  void bgp_router_dump_rib(SLogStream * pStream, SBGPRouter * pRouter);
  // ----- bgp_router_dump_rib_address ------------------------------
  void bgp_router_dump_rib_address(SLogStream * pStream,
				   SBGPRouter * pRouter,
				   net_addr_t tAddr);
  // ----- bgp_router_dump_rib_prefix -------------------------------
  void bgp_router_dump_rib_prefix(SLogStream * pStream,
					 SBGPRouter * pRouter,
					 SPrefix sPrefix);
  // ----- bgp_router_dump_adjrib -----------------------------------
  void bgp_router_dump_adjrib(SLogStream * pStream,
			      SBGPRouter * pRouter,
			      SBGPPeer * pPeer, SPrefix sPrefix,
			      int iInOut);

  ///////////////////////////////////////////////////////////////////
  // LOAD/SAVE FUNCTIONS
  ///////////////////////////////////////////////////////////////////

  // ----- bgp_router_load_rib --------------------------------------
  int bgp_router_load_rib(SBGPRouter * pRouter, const char * pcFileName,
			  uint8_t uFormat, uint8_t uOptions);
#ifdef __EXPERIMENTAL__
  // ----- bgp_router_load_ribs_in ----------------------------------
  int bgp_router_load_ribs_in(SBGPRouter * pRoutes, const char * pcFileName);
#endif
  // ----- bgp_router_save_rib --------------------------------------
  int bgp_router_save_rib(SBGPRouter * pRouter, const char * pcFileName);


  ///////////////////////////////////////////////////////////////////
  // MISCELLANEOUS FUNCTIONS
  ///////////////////////////////////////////////////////////////////

  // -----[ bgp_router_show_stats ]----------------------------------
  void bgp_router_show_stats(SLogStream * pStream, SBGPRouter * pRouter);
  // -----[ bgp_router_show_routes_info ]----------------------------
  int bgp_router_show_routes_info(SLogStream * pStream,
				  SBGPRouter * pRouter,
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
