// ==================================================================
// @(#)bgp_debug.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 09/04/2004
// @lastdate 09/04/2004
// ==================================================================

#ifndef __BGP_DEBUG_H__
#define __BGP_DEBUG_H__

#include <bgp/as_t.h>
#include <net/prefix.h>
#include <stdio.h>

// ----- bgp_debug_dp -----------------------------------------------
extern void bgp_debug_dp(FILE * pStream, SBGPRouter * pRouter,
			 SPrefix sPrefix);

#endif
