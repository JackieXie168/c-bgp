// ==================================================================
// @(#)igp.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 23/02/2004
// @lastdate 23/02/2004
// ==================================================================

#ifndef __NET_IGP_H__
#define __NET_IGP_H__

#include <net/prefix.h>
#include <net/network.h>

// ----- igp_compute_prefix -----------------------------------------
extern int igp_compute_prefix(SNetwork * pNetwork, SNetNode * pNode,
			      SPrefix sPrefix);

#endif

