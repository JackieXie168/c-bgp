// ==================================================================
// @(#)tie_breaks.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 28/07/2003
// $Id: tie_breaks.h,v 1.3 2009-03-24 15:54:50 bqu Exp $
// ==================================================================

#ifndef __BGP_TIE_BREAKS_H__
#define __BGP_TIE_BREAKS_H__

#include <bgp/types.h>

// ----- FTieBreakFunction ------------------------------------------
/**
 * Compare two routes and return
 * (1) if ROUTE1 is prefered over ROUTE2,
 * (-1) if ROUTE2 is prefered over ROUTE1,
 * (0) if no route is prefered over the other.
 */
typedef int (*FTieBreakFunction)(bgp_route_t * route1, bgp_route_t * route2);

#define TIE_BREAK_DEFAULT tie_break_next_hop

// ----- tie_break_next_hop -----------------------------------------
extern int tie_break_next_hop(bgp_route_t * route1, bgp_route_t * toute2);
// ----- tie_break_hash ---------------------------------------------
/*extern int tie_break_hash(bgp_route_t * pRoute1, bgp_route_t * pRoute2);*/
// ----- tie_break_low_ISP ------------------------------------------
/*extern int tie_break_low_ISP(bgp_route_t * pRoute1, bgp_route_t * pRoute2);*/
// ----- tie_break_high_ISP -----------------------------------------
/*extern int tie_break_high_ISP(bgp_route_t * pRoute1, bgp_route_t * pRoute2);*/

#endif
