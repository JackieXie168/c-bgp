// ==================================================================
// @(#)dp_rules.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 13/11/2002
// @lastdate 13/02/2005
// ==================================================================

#ifndef __BGP_DP_RULES_H__
#define __BGP_DP_RULES_H__

#include <libgds/array.h>

#include <bgp/as_t.h>
#include <bgp/route_t.h>
#include <bgp/routes_list.h>

#define BGP_MED_TYPE_DETERMINISTIC  0
#define BGP_MED_TYPE_ALWAYS_COMPARE 1

// ----- FDPRouteCompare -----
// Decision Process Route comparison function prototype
// MUST return:
//   0 if routes are equal
//   > 0 if route1 > route2
//   < 0 if route1 < route2
// Note: function of this type MUST be deterministic, i.e. the
// ordering they define must be complete
typedef int (*FDPRouteCompare)(SBGPRouter * pRouter,
			       SRoute * pRoute1,
			       SRoute * pRoute2);

// ----- bgp_dp_rule_local ------------------------------------------
extern void bgp_dp_rule_local(SBGPRouter * pRouter, SRoutes * pRoutes);

// ----- dp_rule_igp_cost -------------------------------------------
extern uint32_t dp_rule_igp_cost(SBGPRouter * pRouter, net_addr_t tNextHop);


// ----- dp_rule_shortest_path --------------------------------------
extern int dp_rule_shortest_path(SPtrArray * pRoutes);
// ----- dp_rule_lowest_origin --------------------------------------
extern int dp_rule_lowest_origin(SPtrArray * pRoutes);
// ----- dp_rule_lowest_med -----------------------------------------
extern int dp_rule_lowest_med(SPtrArray * pRoutes);
// ----- dp_rule_ebgp_over_ibgp -------------------------------------
extern int dp_rule_ebgp_over_ibgp(SBGPRouter * pRouter, SPtrArray * pRoutes);
// ----- dp_rule_nearest_next_hop -----------------------------------
extern int dp_rule_nearest_next_hop(SBGPRouter * pRouter, SPtrArray * pRoutes);
// ----- dp_rule_shortest_cluster_list ------------------------------
extern int dp_rule_shortest_cluster_list(SBGPRouter * pRouter, SPtrArray * pRoutes);
// ----- dp_rule_lowest_neighbor_address ----------------------------
extern int dp_rule_lowest_neighbor_address(SBGPRouter * pRouter, SPtrArray * pRoutes);
// ----- dp_rule_final ----------------------------------------------
extern int dp_rule_final(SBGPRouter * pRouter, SPtrArray * pRoutes);
// ----- dp_rule_lowest_delay ---------------------------------------
extern int dp_rule_lowest_delay(SPtrArray * pRoutes);

#endif
