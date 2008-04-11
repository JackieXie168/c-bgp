// ==================================================================
// @(#)igp.h
//
// Simple model of an IGP routing protocol. The model does not require
// exchange of messages. It directly compute the routes based on the
// whole topology.
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 23/02/2004
// $Id: igp.h,v 1.4 2008-04-11 11:03:06 bqu Exp $
// ==================================================================

#ifndef __NET_IGP_H__
#define __NET_IGP_H__

#include <net/igp_domain.h>
#include <net/network.h>
#include <net/prefix.h>

#ifdef __cplusplus
extern "C" {
#endif

  // ----- igp_compute_domain ---------------------------------------
  int igp_compute_domain(igp_domain_t * domain);

#ifdef __cplusplus
}
#endif

#endif /* __NET_IGP_H__ */

