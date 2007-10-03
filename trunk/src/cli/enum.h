// ==================================================================
// @(#)enum.h
//
// Enumeration functions used by the CLI.
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be), 
//
// @date 27/04/2007
// @lastdate 01/10/2007
// ==================================================================

#ifndef __CLI_ENUM_H__
#define __CLI_ENUM_H__

#include <bgp/as_t.h>
#include <net/net_types.h>

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ cli_enum_net_nodes ]-------------------------------------
  SNetNode * cli_enum_net_nodes(const char * pcText, int state);
  // -----[ cli_enum_bgp_routers ]-----------------------------------
  SBGPRouter * cli_enum_bgp_routers(const char * pcText, int state);
  // -----[ cli_enum_bgp_peers ]---------------------------------------
  SBGPPeer * cli_enum_bgp_peers(SBGPRouter * pRouter, const char * pcText,
				int state);
  // -----[ cli_enum_net_nodes_addr ]--------------------------------
  char * cli_enum_net_nodes_addr(const char * pcText, int state);
  // -----[ cli_enum_bgp_routers_addr ]------------------------------
  char * cli_enum_bgp_routers_addr(const char * pcText, int state);

#ifdef __cplusplus
}
#endif

#endif /** __CLI_ENUM_H__ */
