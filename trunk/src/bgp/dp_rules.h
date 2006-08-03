// ==================================================================
// @(#)dp_rules.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 13/11/2002
// @lastdate 15/11/2005
// ==================================================================

#ifndef __BGP_DP_RULES_H__
#define __BGP_DP_RULES_H__

#include <libgds/array.h>

#include <bgp/as_t.h>
#include <bgp/route_t.h>
#include <bgp/routes_list.h>

#define BGP_MED_TYPE_DETERMINISTIC  0
#define BGP_MED_TYPE_ALWAYS_COMPARE 1

// ----- FDPRule -----
/**
 * Defines a decision process rule. A rule takes as arguments a router
 * and a set of possible routes. It removes routes from the set.
 * Note: the int return value is not used yet. Decision process rules
 * must return 0 for now.
 */
typedef int (*FDPRule)(SBGPRouter * pRouter, SRoutes * pRoutes);

// ----- FDPRouteCompare -----
/**
 * Defines a route comparison function prototype. The function must
 * return:
 *   0 if routes are equal
 *   > 0 if route1 > route2
 *   < 0 if route1 < route2
 * Note: function of this type MUST be deterministic, i.e. the
 * ordering they define must be complete
 */
typedef int (*FDPRouteCompare)(SBGPRouter * pRouter,
			       SRoute * pRoute1,
			       SRoute * pRoute2);

int dp_rule_no_selection(SBGPRouter * pRouter, SRoutes * pRoutes);
int dp_rule_lowest_nh(SBGPRouter * pRouter, SRoutes * pRoutes);
// ----- bgp_dp_rule_local ------------------------------------------
extern void bgp_dp_rule_local(SBGPRouter * pRouter, SRoutes * pRoutes);

// ----- dp_rule_highest_pref ---------------------------------------
extern int dp_rule_highest_pref(SBGPRouter * pRouter, SRoutes * pRoutes);
// ----- dp_rule_shortest_path --------------------------------------
extern int dp_rule_shortest_path(SBGPRouter * pRouter, SRoutes * pRoutes);
// ----- dp_rule_lowest_origin --------------------------------------
extern int dp_rule_lowest_origin(SBGPRouter * pRouter, SRoutes * pRoutes);
// ----- dp_rule_lowest_med -----------------------------------------
extern int dp_rule_lowest_med(SBGPRouter * pRouter, SRoutes * pRoutes);
// ----- dp_rule_ebgp_over_ibgp -------------------------------------
extern int dp_rule_ebgp_over_ibgp(SBGPRouter * pRouter, SPtrArray * pRoutes);
// ----- dp_rule_nearest_next_hop -----------------------------------
extern int dp_rule_nearest_next_hop(SBGPRouter * pRouter, SRoutes * pRoutes);
// ----- dp_rule_lowest_router_id -----------------------------------
extern int dp_rule_lowest_router_id(SBGPRouter * pRouter, SRoutes * pRoutes);
// ----- dp_rule_shortest_cluster_list ------------------------------
extern int dp_rule_shortest_cluster_list(SBGPRouter * pRouter,
					 SRoutes * pRoutes);
// ----- dp_rule_lowest_neighbor_address ----------------------------
extern int dp_rule_lowest_neighbor_address(SBGPRouter * pRouter,
					   SRoutes * pRoutes);

// ----- dp_rule_final ----------------------------------------------
extern int dp_rule_final(SBGPRouter * pRouter, SRoutes * pRoutes);
// ----- dp_rule_lowest_delay ---------------------------------------
extern int dp_rule_lowest_delay(SBGPRouter * pRouter, SRoutes * pRoutes);

#endif
