// ==================================================================
// @(#)net.h
//
// @author Bruno Quoitin (bqu@infonet.fundp.ac.be)
// @date 15/07/2003
// @lastdate 22/07/2003
// ==================================================================

#ifndef __CLI_NET_H__
#define __CLI_NET_H__

#include <libgds/cli.h>
#include <net/network.h>

// ----- cli_net_node_by_addr ---------------------------------------
extern SNetNode * cli_net_node_by_addr(char * pcAddr);
// ----- cli_register_net -------------------------------------------
extern int cli_register_net(SCli * pCli);

#endif
