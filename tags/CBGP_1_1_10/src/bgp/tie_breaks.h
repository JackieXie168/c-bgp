// ==================================================================
// @(#)tie_breaks.h
//
// @author Bruno Quoitin (bqu@infonet.fundp.ac.be)
// @date 28/07/2003
// @lastdate 28/07/2003
// ==================================================================

#ifndef __BGP_TIE_BREAKS_H__
#define __BGP_TIE_BREAKS_H__

#include <bgp/route_t.h>

// ----- FTieBreakFunction ------------------------------------------
/**
 * Compare two routes and return
 * (1) if ROUTE1 is prefered over ROUTE2,
 * (-1) if ROUTE2 is prefered over ROUTE1,
 * (0) if no route is prefered over the other.
 */
typedef int (*FTieBreakFunction)(SRoute * pRoute1, SRoute * pRoute2);

#define TIE_BREAK_DEFAULT tie_break_hash

// ----- tie_break_hash ---------------------------------------------
extern int tie_break_hash(SRoute * pRoute1, SRoute * pRoute2);
// ----- tie_break_low_ISP ------------------------------------------
extern int tie_break_low_ISP(SRoute * pRoute1, SRoute * pRoute2);
// ----- tie_break_high_ISP -----------------------------------------
extern int tie_break_high_ISP(SRoute * pRoute1, SRoute * pRoute2);

#endif
