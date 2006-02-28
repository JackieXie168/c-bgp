// ==================================================================
// @(#)as.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 22/11/2002
// @lastdate 15/11/2005
// ==================================================================

#ifndef __AS_H__
#define __AS_H__

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

#define AS_RECORD_ROUTE_SUCCESS  0
#define AS_RECORD_ROUTE_LOOP     1
#define AS_RECORD_ROUTE_TOO_LONG 2
#define AS_RECORD_ROUTE_UNREACH  3

#define BGP_NLRI_BE        0
#define BGP_NLRI_QOS_DELAY 1

extern unsigned long route_create_count;
extern unsigned long route_copy_count;
extern unsigned long route_destroy_count;

extern FTieBreakFunction BGP_OPTIONS_TIE_BREAK;
extern uint8_t BGP_OPTIONS_NLRI;
extern uint32_t BGP_OPTIONS_DEFAULT_LOCAL_PREF;
extern uint8_t BGP_OPTIONS_MED_TYPE;
extern uint8_t BGP_OPTIONS_SHOW_MODE;
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
#define DP_NUM_RULES 9
extern char * DP_RULE_NAME[DP_NUM_RULES];
extern FDPRule DP_RULES[DP_NUM_RULES];

// ----- bgp_router_create ------------------------------------------
extern SBGPRouter * bgp_router_create(uint16_t uNumber,
				      SNetNode * pNode,
				      uint32_t uTBID);
// ----- bgp_router_destroy -----------------------------------------
extern void bgp_router_destroy(SBGPRouter ** ppRouter);
// ----- bgp_router_set_name ----------------------------------------
void bgp_router_set_name(SBGPRouter * pRouter, char * pcName);
// ----- bgp_router_get_name ----------------------------------------
char * bgp_router_get_name(SBGPRouter * pRouter);
// ----- bgp_router_add_peer ----------------------------------------
extern SBGPPeer * bgp_router_add_peer(SBGPRouter * pRouter,
				      uint16_t uRemoteAS,
				      net_addr_t tAddr,
				      uint8_t uPeerType);
// ----- bgp_router_find_peer ---------------------------------------
extern SPeer * bgp_router_find_peer(SBGPRouter * pRouter, net_addr_t tAddr);
// ----- bgp_router_peer_set_filter ---------------------------------
extern int bgp_router_peer_set_filter(SBGPRouter * pRouter, net_addr_t tAddr,
				      SFilter * pFilter, int iIn);
// ----- bgp_router_add_network -------------------------------------
extern int bgp_router_add_network(SBGPRouter * pRouter, SPrefix sPrefix);
// ----- bgp_router_del_network -------------------------------------
extern int bgp_router_del_network(SBGPRouter * pRouter, SPrefix sPrefix);
// ----- as_add_qos_network -----------------------------------------
extern int as_add_qos_network(SBGPRouter * pRouter, SPrefix sPrefix,
			      net_link_delay_t tDelay);
// ----- bgp_router_start -------------------------------------------
extern int bgp_router_start(SBGPRouter * pRouter);
// ----- bgp_router_stop --------------------------------------------
extern int bgp_router_stop(SBGPRouter * pRouter);
// ----- bgp_router_dump_adj_rib_in ---------------------------------
extern int bgp_router_dump_adj_rib_in(FILE * pStream, SBGPRouter * pRouter,
				      SPrefix sPrefix);
// ----- bgp_router_dump_rt_dp_rule ---------------------------------
extern int bgp_router_dump_rt_dp_rule(FILE * pStream, SBGPRouter * pRouter,
				      SPrefix sPrefix);
// ----- bgp_router_info --------------------------------------------
extern void bgp_router_info(FILE * pStream, SBGPRouter * pRouter);
/*// ----- bgp_router_decision_process_dop ------------------------------------
extern void bgp_router_decision_process_dop(SBGPRouter * pRouter,
SPtrArray * pRoutes);*/
// ----- bgp_router_decision_process_disseminate_to_peer ------------
extern void bgp_router_decision_process_disseminate_to_peer(SBGPRouter * pRouter,
							    SPrefix sPrefix,
							    SRoute * pRoute,
							    SPeer * pPeer);
// ----- bgp_router_decision_process_disseminate --------------------
extern void bgp_router_decision_process_disseminate(SBGPRouter * pRouter,
						    SPrefix sPrefix,
						    SRoute * pRoute);
// ----- bgp_router_get_best_routes ---------------------------------
extern SRoutes * bgp_router_get_best_routes(SBGPRouter * pRouter,
					    SPrefix sPrefix);
// ----- bgp_router_get_feasible_routes -----------------------------
#ifdef __EXPERIMENTAL_ADVERTISE_BEST_EXTERNAL_TO_INTERNAL__
extern SRoutes * bgp_router_get_feasible_routes(SBGPRouter * pRouter,
						SPrefix sPrefix, uint8_t uEBGPRoute);
#else
extern SRoutes * bgp_router_get_feasible_routes(SBGPRouter * pRouter,
						SPrefix sPrefix);
#endif
// ----- bgp_router_decision_process --------------------------------
extern int bgp_router_decision_process(SBGPRouter * pRouter,
				       SPeer * pOriginPeer,
				       SPrefix sPrefix);
// ----- bgp_router_handle_message ----------------------------------
extern int bgp_router_handle_message(void * pRouter, SNetMessage * pMessage);
// ----- bgp_router_ecomm_red_process -------------------------------
extern int bgp_router_ecomm_red_process(SPeer * pPeer, SRoute * pRoute);
// ----- bgp_router_num_providers -----------------------------------
extern uint16_t bgp_router_num_providers(SBGPRouter * pRouter);
// ----- bgp_router_dump_id -----------------------------------------
extern void bgp_router_dump_id(FILE * pStream, SBGPRouter * pRouter);
// ----- bgp_router_load_rib ----------------------------------------
extern int bgp_router_load_rib(char * pcFileName, SBGPRouter * pRouter);
#ifdef __EXPERIMENTAL__
// ----- bgp_router_load_ribs_in ------------------------------------
int bgp_router_load_ribs_in(char * pcFileName, SBGPRouter * pRoutes);
#endif

// -----[ bgp_router_rerun ]-----------------------------------------
extern int bgp_router_rerun(SBGPRouter * pRouter, SPrefix sPrefix);
// -----[ bgp_router_peer_readv_prefix ]-----------------------------
extern int bgp_router_peer_readv_prefix(SBGPRouter * pRouter,
					SPeer * pPeer,
					SPrefix sPrefix);

// ----- bgp_router_reset -------------------------------------------
extern int bgp_router_reset(SBGPRouter * pRouter);
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
// ----- bgp_router_walton_peer_set ----------------------------------
int bgp_router_walton_peer_set(SPeer * pPeer, unsigned int uWaltonLimit);
#endif

// ----- bgp_router_scan_rib ----------------------------------------
int bgp_router_scan_rib(SBGPRouter * pRouter);

// ----- bgp_router_dump_networks -----------------------------------
extern void bgp_router_dump_networks(FILE * pStream, SBGPRouter * pRouter);
// ----- bgp_router_dump_peers --------------------------------------
extern void bgp_router_dump_peers(FILE * pStream, SBGPRouter * pRouter);
// ----- bgp_router_peers_for_each ----------------------------------
extern int bgp_router_peers_for_each(SBGPRouter * pRouter,
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
// ----- _bgp_router_peers_compare -----------------------------------
int _bgp_router_peers_compare(void * pItem1, void * pItem2,
    unsigned int uEltSize);
// ----- bgp_router_dump_rib_string ----------------------------------------
char * bgp_router_dump_rib_string(SBGPRouter * pRouter);
// ----- bgp_router_dump_rib ----------------------------------------
extern void bgp_router_dump_rib(FILE * pStream, SBGPRouter * pRouter);
// ----- bgp_router_dump_rib_address --------------------------------
extern void bgp_router_dump_rib_address(FILE * pStream, SBGPRouter * pRouter,
					net_addr_t tAddr);
// ----- bgp_router_dump_rib_prefix ---------------------------------
extern void bgp_router_dump_rib_prefix(FILE * pStream, SBGPRouter * pRouter,
				       SPrefix sPrefix);
// ----- bgp_router_dump_adjrib -------------------------------------
extern void bgp_router_dump_adjrib(FILE * pStream, SBGPRouter * pRouter,
				   SPeer * pPeer, SPrefix sPrefix,
				   int iInOut);
// ----- bgp_router_save_rib ----------------------------------------
extern int bgp_router_save_rib(char * pcFileName, SBGPRouter * pRouter);
// -----[ bgp_router_show_stats ]------------------------------------
extern void bgp_router_show_stats(FILE * pStream, SBGPRouter * pRouter);
// -----[ bgp_router_show_route_info ]-------------------------------
extern int bgp_router_show_route_info(FILE * pStream, SBGPRouter * pRouter,
				      SPrefix sPrefix);

// ----- bgp_router_record_route ------------------------------------
extern int bgp_router_record_route(SBGPRouter * pRouter,
				   SPrefix sPrefix, SBGPPath ** ppPath,
				   int iPreserveDups);
// ----- bgp_router_record_route_bounded_match ----------------------
extern int bgp_router_record_route_bounded_match(SBGPRouter * pRouter,
						 SPrefix sPrefix,
						 uint8_t uBound,
						 SBGPPath ** ppPath,
						 int iPreserveDups);
// ----- bgp_router_dump_recorded_route -----------------------------
extern void bgp_router_dump_recorded_route(FILE * pStream,
					   SBGPRouter * pRouter,
					   SPrefix sPrefix,
					   SBGPPath * pPath,
					   int iResult);

#endif
