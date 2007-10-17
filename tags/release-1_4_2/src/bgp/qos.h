// ==================================================================
// @(#)qos.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 13/11/2002
// @lastdate 18/05/2004
// ==================================================================

#ifndef __BGP_QOS_H__
#define __BGP_QOS_H__

#include <libgds/array.h>

#include <bgp/as.h>
#include <bgp/peer_t.h>
#include <bgp/qos_t.h>
#include <bgp/route_t.h>
#include <net/prefix.h>

#ifdef BGP_QOS

extern unsigned int BGP_OPTIONS_QOS_AGGR_LIMIT;

// ----- qos_route_aggregate ----------------------------------------
extern SRoute * qos_route_aggregate(SPtrArray * pRoutes,
				    SRoute * pBestRoute);
// ----- qos_route_init_delay ---------------------------------------
extern void qos_route_init_delay(SRoute * pRoute);
// ----- qos_route_init_bandwidth -----------------------------------
extern void qos_route_init_bandwidth(SRoute * pRoute);
// ----- qos_route_update_delay -------------------------------------
extern int qos_route_update_delay(SRoute * pRoute, net_link_delay_t uDelay);
// ----- qos_route_update_bandwidth ---------------------------------
extern int qos_route_update_bandwidth(SRoute * pRoute,
				      uint32_t uBandwidth);
// ----- qos_route_delay_equals -------------------------------------
extern int qos_route_delay_equals(SRoute * pRoute1, SRoute * pRoute2);
// ----- qos_route_bandwidth_equals ---------------------------------
extern int qos_route_bandwidth_equals(SRoute * pRoute1, SRoute * pRoute2);
// ----- qos_advertise_to_peer --------------------------------------
extern int qos_advertise_to_peer(SAS * pAS, SPeer * pPeer,
				 SRoute * pRoute);
// ----- qos_decision_process_tie_break -----------------------------
extern void qos_decision_process_tie_break(SAS * pAS,
					   SPtrArray * pRoutes,
					   unsigned int uNumRoutes);
// ----- qos_decision_process ---------------------------------------
extern int qos_decision_process(SAS * pAS, SPeer * pOriginPeer,
				SPrefix sPrefix);

void _qos_test();

#endif

#endif
