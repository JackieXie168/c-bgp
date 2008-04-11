// ==================================================================
// @(#)bgp_debug.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 09/04/2004
// $Id: bgp_debug.h,v 1.3 2008-04-11 11:03:06 bqu Exp $
// ==================================================================

#ifndef __BGP_DEBUG_H__
#define __BGP_DEBUG_H__

#include <bgp/as_t.h>
#include <net/prefix.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

  // ----- bgp_debug_dp -----------------------------------------------
  void bgp_debug_dp(SLogStream * stream, bgp_router_t * router,
		    ip_pfx_t prefix);

#ifdef __cplusplus
}
#endif

#endif
