// ==================================================================
// @(#)igp.h
//
// Simple model of an IGP routing protocol. The model does not require
// exchange of messages. It directly compute the routes based on the
// whole topology.
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 23/02/2004
// @lastdate 01/08/2005
// ==================================================================

#ifndef __NET_IGP_H__
#define __NET_IGP_H__

#include <net/igp_domain.h>
#include <net/network.h>
#include <net/prefix.h>

// ----- igp_compute_domain -----------------------------------------
extern int igp_compute_domain(SIGPDomain * pDomain);

#endif

