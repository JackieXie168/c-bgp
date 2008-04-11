// ==================================================================
// @(#)net.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 15/07/2003
// $Id: net.h,v 1.4 2008-04-11 11:03:06 bqu Exp $
// ==================================================================

#ifndef __CLI_NET_H__
#define __CLI_NET_H__

#include <libgds/cli.h>
#include <net/network.h>

#ifdef __cplusplus
extern "C" {
#endif

  // ----- cli_register_net -----------------------------------------
  int cli_register_net(SCli * pCli);

#ifdef __cplusplus
}
#endif

#endif /* __CLI_NET_H__ */
