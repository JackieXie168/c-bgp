// ==================================================================
// @(#)bgp_debug.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 09/04/2004
// $Id: bgp_debug.h,v 1.4 2009-03-24 14:10:35 bqu Exp $
// ==================================================================

#ifndef __BGP_DEBUG_H__
#define __BGP_DEBUG_H__

#include <stdio.h>

#include <libgds/stream.h>

#include <net/prefix.h>
#include <bgp/types.h>

#ifdef __cplusplus
extern "C" {
#endif

  // ----- bgp_debug_dp -----------------------------------------------
  void bgp_debug_dp(gds_stream_t * stream, bgp_router_t * router,
		    ip_pfx_t prefix);

#ifdef __cplusplus
}
#endif

#endif /* __BGP_DEBUG_H__ */
