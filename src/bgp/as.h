// ==================================================================
// @(#)as.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 22/11/2002
// @lastdate 27/02/2004
// ==================================================================

#ifndef __AS_H__
#define __AS_H__

#include <stdio.h>

#include <libgds/array.h>
#include <libgds/types.h>
#include <libgds/list.h>

#include <bgp/as_t.h>
#include <bgp/filter.h>
#include <bgp/message.h>
#include <bgp/peer_t.h>
#include <bgp/rib.h>
#include <bgp/route.h>
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

#define MAX_AS 65536

#define AS_LOG_DEBUG(AS, M) \
  LOG_DEBUG("AS%u:", AS->uNumber); \
  LOG_ENABLED_DEBUG(); \
  ip_address_dump(log_get_stream(pMainLog), AS->pNode->tAddr); \
  LOG_DEBUG(M);

// ----- as_create --------------------------------------------------
extern SAS * as_create(uint16_t uNumber, SNetNode * pNode,
		       uint32_t uTBID);
// ----- as_destroy -------------------------------------------------
extern void as_destroy(SAS ** ppAS);
// ----- as_add_peer ------------------------------------------------
extern int as_add_peer(SAS * pAS, uint16_t uRemoteAS,
		       net_addr_t tAddr, uint8_t uPeerType);
// ----- as_find_peer -----------------------------------------------
extern SPeer * as_find_peer(SAS * pAS, net_addr_t tAddr);
// ----- as_peer_set_in_filter --------------------------------------
extern int as_peer_set_in_filter(SAS * pAS, net_addr_t tAddr,
				 SFilter * pFilter);
// ----- as_peer_set_out_filter --------------------------------------
extern int as_peer_set_out_filter(SAS * pAS, net_addr_t tAddr,
				  SFilter * pFilter);
// ----- as_add_route -----------------------------------------------
extern int as_add_network(SAS * pAS, SPrefix sPrefix);
// ----- as_add_qos_network -----------------------------------------
extern int as_add_qos_network(SAS * pAS, SPrefix sPrefix,
			      net_link_delay_t tDelay);
// ----- as_run -----------------------------------------------------
extern int as_run(SAS * pAS);
// ----- as_dump_adj_rib_in -----------------------------------------
extern int as_dump_adj_rib_in(FILE * pStream, SAS * pAS,
			      SPrefix sPrefix);
// ----- as_dump_rt_dp_rule -----------------------------------------
extern int as_dump_rt_dp_rule(FILE * pStream, SAS * pAS,
			      SPrefix sPrefix);
// ----- as_decision_process_dop ------------------------------------
extern void as_decision_process_dop(SAS * pAS, SPtrArray * pRoutes);
// ----- as_decision_process_disseminate_to_peer --------------------
extern void as_decision_process_disseminate_to_peer(SAS * pAS,
						    SPrefix sPrefix,
						    SRoute * pRoute,
						    SPeer * pPeer);
// ----- as_decision_process_disseminate ----------------------------
extern void as_decision_process_disseminate(SAS * pAS,
					    SPrefix sPrefix,
					    SRoute * pRoute);
// ----- as_decision_process ----------------------------------------
extern int as_decision_process(SAS * pAS, SPeer * pOriginPeer,
			       SPrefix sPrefix);
// ----- as_handle_message ------------------------------------------
extern int as_handle_message(void * pAS, SNetMessage * pMessage);
// ----- as_record_route --------------------------------------------
extern int as_record_route(FILE * pStream, SAS * pAS,
			   SPrefix sPrefix, SPath ** ppPath);
// ----- as_ecomm_red_process ---------------------------------------
extern int as_ecomm_red_process(SPeer * pPeer, SRoute * pRoute);
// ----- as_num_providers -------------------------------------------
extern uint16_t as_num_providers(SAS * pAS);
// ----- as_dump_id -------------------------------------------------
extern void as_dump_id(FILE * pStream, SAS * pAS);
// ----- as_load_rib ------------------------------------------------
extern int as_load_rib(char * pcFileName, SAS * pAS);

// ----- bgp_router_dump_networks -----------------------------------
extern void bgp_router_dump_networks(FILE * pStream, SBGPRouter * pRouter);
// ----- bgp_router_dump_peers --------------------------------------
extern void bgp_router_dump_peers(FILE * pStream, SBGPRouter * pRouter);
// ----- bgp_router_dump_rib ----------------------------------------
extern void bgp_router_dump_rib(FILE * pStream, SBGPRouter * pRouter);
// ----- bgp_router_dump_rib_address --------------------------------
extern void bgp_router_dump_rib_address(FILE * pStream, SBGPRouter * pRouter,
					net_addr_t tAddr);
// ----- bgp_router_dump_rib_prefix ---------------------------------
extern void bgp_router_dump_rib_prefix(FILE * pStream, SBGPRouter * pRouter,
				       SPrefix sPrefix);
// ----- bgp_router_dump_ribin --------------------------------------
extern void bgp_router_dump_ribin(FILE * pStream, SBGPRouter * pRouter,
				  SPeer * pPeer, SPrefix sPrefix);
// ----- bgp_router_save_rib ----------------------------------------
extern int bgp_router_save_rib(char * pcFileName, SBGPRouter * pRouter);

#endif
