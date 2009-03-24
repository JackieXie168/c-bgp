// ==================================================================
// @(#)common.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 15/07/2003
// $Id: common.h,v 1.7 2009-03-24 15:58:43 bqu Exp $
// ==================================================================

#ifndef __CLI_COMMON_H__
#define __CLI_COMMON_H__

#include <libgds/cli.h>
#include <net/net_types.h>
#include <net/prefix.h>
#include <bgp/types.h>

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ str2node ]-----------------------------------------------
  int str2node(const char * addr, net_node_t ** node);
  // -----[ str2node_id ]--------------------------------------------
  int str2node_id(const char * id, net_node_t ** node);
  // -----[ str2subnet ]---------------------------------------------
  int str2subnet(const char * str, net_subnet_t ** subnet);

  // -----[ cli_arg_file ]-------------------------------------------
  cli_arg_t * cli_arg_file(const char * name, cli_arg_check_f check);

  // ----- cli_get --------------------------------------------------
  cli_t * cli_get();
  
  // -----[ _cli_common_init ]---------------------------------------
  void _cli_common_init();
  // ----- _cli_destroy ---------------------------------------------
  void _cli_common_destroy();

  // -----[ cli_set_param ]------------------------------------------
  void cli_set_param(const char * param, const char * value);
  // -----[ cli_get_param ]------------------------------------------
  const char * cli_get_param(const char * param);
  
#ifdef __cplusplus
}
#endif

#endif /* __CLI_COMMON_H__ */
