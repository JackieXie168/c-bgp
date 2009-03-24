// ==================================================================
// @(#)enum.h
//
// Enumeration functions used by the CLI.
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be), 
//
// @date 27/04/2007
// $Id: enum.h,v 1.6 2009-03-24 15:58:43 bqu Exp $
// ==================================================================

#ifndef __CLI_ENUM_H__
#define __CLI_ENUM_H__

#include <net/net_types.h>
#include <bgp/types.h>

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ cli_enum_net_nodes ]-------------------------------------
  net_node_t * cli_enum_net_nodes(const char * text, int state);
  // -----[ cli_enum_bgp_routers ]-----------------------------------
  bgp_router_t * cli_enum_bgp_routers(const char * text, int state);
  // -----[ cli_enum_bgp_peers ]-------------------------------------
  bgp_peer_t * cli_enum_bgp_peers(const char * text, int state);

  // -----[ cli_enum_net_nodes_addr ]--------------------------------
  char * cli_enum_net_nodes_addr(const char * text, int state);
  // -----[ cli_enum_bgp_routers_addr ]------------------------------
  char * cli_enum_bgp_routers_addr(const char * text, int state);
  // -----[ cli_enum_bgp_peers_addr ]--------------------------------
  char * cli_enum_bgp_peers_addr(const char * text, int state);

  // -----[ cli_enum_ctx_bgp_router ]--------------------------------
  void cli_enum_ctx_bgp_router(bgp_router_t * router);


#ifdef __cplusplus
}
#endif

#endif /** __CLI_ENUM_H__ */
