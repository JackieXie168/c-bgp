// ==================================================================
// @(#)bgp.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 15/07/2003
// $Id: bgp.h,v 1.5 2009-03-24 15:57:51 bqu Exp $
// ==================================================================

#ifndef __CLI_BGP_H__
#define __CLI_BGP_H__

#include <libgds/cli.h>

#ifdef __cplusplus
extern "C" {
#endif

  // ----- cli_register_bgp -----------------------------------------
  void cli_register_bgp(cli_cmd_t * parent);

#ifdef __cplusplus
}
#endif

#endif /* __CLI_BGP_H__ */
