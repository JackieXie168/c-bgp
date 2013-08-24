// ==================================================================
// @(#)auto-config.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 15/11/2005
// $Id: auto-config.h,v 1.3 2009-03-24 13:56:45 bqu Exp $
// ==================================================================

#ifndef __BGP_AUTO_CONFIG_H__
#define __BGP_AUTO_CONFIG_H__

#include <net/prefix.h>
#include <bgp/types.h>

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ bgp_auto_config_session ]--------------------------------
  int bgp_auto_config_session(bgp_router_t * router,
			      net_addr_t remote_addr,
			      uint16_t remote_asn,
			      bgp_peer_t ** peer_ref);

#ifdef __cplusplus
}
#endif

#endif /* __BGP_AUTO_CONFIG_H__ */
