// ==================================================================
// @(#)dp_rules.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 13/11/2002
// @lastdate 10/03/2008
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
typedef int (*FDPRule)(bgp_router_t * pRouter, bgp_routes_t * pRoutes);

struct dp_rule_t {
  char  * pcName;
  FDPRule fRule;
};

// -----[ decision process ]-----------------------------------------
#define DP_NUM_RULES 11
extern struct dp_rule_t DP_RULES[DP_NUM_RULES];

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
typedef int (*FDPRouteCompare)(bgp_router_t * pRouter,
			       bgp_route_t * pRoute1,
			       bgp_route_t * pRoute2);

#ifdef __cplusplus
extern "C" {
#endif 

  int dp_rule_no_selection(bgp_router_t * pRouter, bgp_routes_t * pRoutes);
  int dp_rule_lowest_nh(bgp_router_t * pRouter, bgp_routes_t * pRoutes);
  int dp_rule_lowest_path(bgp_router_t * pRouter, bgp_routes_t * pRoutes);
  // ----- bgp_dp_rule_local ----------------------------------------
  void bgp_dp_rule_local(bgp_router_t * pRouter, bgp_routes_t * pRoutes);
  // ----- dp_rule_highest_pref -------------------------------------
  int dp_rule_highest_pref(bgp_router_t * pRouter, bgp_routes_t * pRoutes);
  // ----- dp_rule_shortest_path ------------------------------------
  int dp_rule_shortest_path(bgp_router_t * pRouter, bgp_routes_t * pRoutes);
  // ----- dp_rule_lowest_origin ------------------------------------
  int dp_rule_lowest_origin(bgp_router_t * pRouter, bgp_routes_t * pRoutes);
  // ----- dp_rule_lowest_med ---------------------------------------
  int dp_rule_lowest_med(bgp_router_t * pRouter, bgp_routes_t * pRoutes);
  // ----- dp_rule_ebgp_over_ibgp -----------------------------------
  int dp_rule_ebgp_over_ibgp(bgp_router_t * pRouter, SPtrArray * pRoutes);
  // ----- dp_rule_nearest_next_hop ---------------------------------
  int dp_rule_nearest_next_hop(bgp_router_t * pRouter, bgp_routes_t * pRoutes);
  // ----- dp_rule_lowest_router_id ---------------------------------
  int dp_rule_lowest_router_id(bgp_router_t * pRouter, bgp_routes_t * pRoutes);
  // ----- dp_rule_shortest_cluster_list ----------------------------
  int dp_rule_shortest_cluster_list(bgp_router_t * pRouter, bgp_routes_t * pRoutes);
  // ----- dp_rule_lowest_neighbor_address --------------------------
  int dp_rule_lowest_neighbor_address(bgp_router_t * pRouter, bgp_routes_t * pRoutes);
  
  // ----- dp_rule_final ----------------------------------------------
  int dp_rule_final(bgp_router_t * pRouter, bgp_routes_t * pRoutes);
  // ----- dp_rule_lowest_delay ---------------------------------------
  int dp_rule_lowest_delay(bgp_router_t * pRouter, bgp_routes_t * pRoutes);

#ifdef __cplusplus
}
#endif

#endif /* __BGP_DP_RULES_H__ */
