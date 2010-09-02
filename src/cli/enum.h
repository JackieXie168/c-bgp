// ==================================================================
// @(#)enum.h
//
// Enumeration functions used by the CLI.
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be), 
//
// @date 27/04/2007
// $Id: enum.h,v 1.7 2009-08-31 09:37:41 bqu Exp $
// ==================================================================

#ifndef __CLI_ENUM_H__
#define __CLI_ENUM_H__

#include <net/net_types.h>
#include <bgp/types.h>

#define BY_NAME 1
#define BY_IP_ADDR 2
#define BY_BOTH 3

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ cli_enum_net_nodes ]-------------------------------------
  net_node_t * cli_enum_net_nodes(const char * text, int state, int by);
  // -----[ cli_enum_net_nodes_id ]----------------------------------
  net_node_t * cli_enum_net_nodes_id(const char * text, int state);
  // -----[ cli_enum_bgp_routers ]-----------------------------------
  bgp_router_t * cli_enum_bgp_routers(const char * text, int state);
  // -----[ cli_enum_bgp_peers ]-------------------------------------
  bgp_peer_t * cli_enum_bgp_peers(const char * text, int state);

  // -----[ cli_enum_net_nodes_addr ]--------------------------------
  char * cli_enum_net_nodes_addr(const char * text, int state);
  // -----[ cli_enum_net_nodes_addr_OR_id ]-----------------------------
  char * cli_enum_net_nodes_addr_OR_id(const char * text, int state);
    // -----[ cli_enum_net_nodes_addr_OR_id_OR_name ]-----------------------------
  char * cli_enum_net_nodes_addr_OR_id_OR_name(const char * text, int state);
    // -----[ cli_enum_net_nodes_addr_OR_name ]-----------------------------
  char * cli_enum_net_nodes_addr_OR_name(const char * text, int state);
    // -----[ cli_enum_bgp_routers_addr ]------------------------------
  char * cli_enum_bgp_routers_addr(const char * text, int state);
  // -----[ cli_enum_bgp_routers_addr_id ]---------------------------
  char * cli_enum_bgp_routers_addr_id(const char * text, int state);
  // -----[ cli_enum_bgp_peers_addr ]--------------------------------
  char * cli_enum_bgp_peers_addr(const char * text, int state);

  // -----[ cli_enum_ctx_bgp_router ]--------------------------------
  void cli_enum_ctx_bgp_router(bgp_router_t * router);


#ifdef __cplusplus
}
#endif

#endif /** __CLI_ENUM_H__ */
