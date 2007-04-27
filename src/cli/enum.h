// ==================================================================
// @(#)enum.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be), 
//
// @date 27/04/2007
// @lastdate 27/04/2007
// ==================================================================

#ifndef __CLI_ENUM_H__
#define __CLI_ENUM_H__

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ cli_enum_net_nodes ]-------------------------------------
  char * cli_enum_net_nodes(const char * pcText, int state);
  // -----[ cli_enum_bgp_routers ]-----------------------------------
  char * cli_enum_bgp_routers(const char * pcText, int state);

#ifdef __cplusplus
}
#endif

#endif /** __CLI_ENUM_H__ */
