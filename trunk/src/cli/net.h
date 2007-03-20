// ==================================================================
// @(#)net.h
//
// @author Bruno Quoitin (bqu@infonet.fundp.ac.be)
// @date 15/07/2003
// @lastdate 16/01/2007
// ==================================================================

#ifndef __CLI_NET_H__
#define __CLI_NET_H__

#include <libgds/cli.h>
#include <net/network.h>

#ifdef __cplusplus
extern "C" {
#endif

  // ----- cli_net_node_by_addr -------------------------------------
  SNetNode * cli_net_node_by_addr(char * pcAddr);
  // ----- cli_register_net -----------------------------------------
  int cli_register_net(SCli * pCli);

#ifdef __cplusplus
}
#endif

#endif /* __CLI_NET_H__ */
