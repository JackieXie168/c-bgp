// ==================================================================
// @(#)net.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 15/07/2003
// $Id: net.h,v 1.5 2009-03-24 15:58:43 bqu Exp $
// ==================================================================

#ifndef __CLI_NET_H__
#define __CLI_NET_H__

#include <libgds/cli.h>
#include <net/network.h>

#ifdef __cplusplus
extern "C" {
#endif

  // ----- cli_register_net -----------------------------------------
  void cli_register_net(cli_cmd_t * parent);

#ifdef __cplusplus
}
#endif

#endif /* __CLI_NET_H__ */
