// ==================================================================
// @(#)auto-config.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 15/11/2005
// @lastdate 15/11/2005
// ==================================================================

#ifndef __BGP_AUTO_CONFIG_H__
#define __BGP_AUTO_CONFIG_H__

#include <bgp/as_t.h>
#include <bgp/peer_t.h>
#include <net/prefix.h>

// -----[ bgp_auto_config_session ]----------------------------------
extern int bgp_auto_config_session(SBGPRouter * pRouter,
				   net_addr_t tRemoteAddr,
				   uint16_t uRemoteAS,
				   SBGPPeer ** ppPeer);

#endif
