// ==================================================================
// @(#)net_ospf.h
//
// @author Stefano Iasi (stefanoia@tin.it)
// @date 15/07/2003
// @lastdate 22/07/2003
// ==================================================================

#ifdef OSPF_SUPPORT

#ifndef __CLI_NET_OSPF_H__
#define __CLI_NET_OSPF_H__


#include <libgds/cli.h>

// ----- cli_net_node_by_addr ---------------------------------------
// extern SNetNode * cli_net_node_by_addr(char * pcAddr);
// ----- cli_register_net -------------------------------------------
// extern int cli_register_net(SCli * pCli);
// ----- cli_register_net_node_ospf --------------------------------------
extern int cli_register_net_node_ospf(SCliCmds * pCmds);
// ----- cli_register_net_ospf --------------------------------------
//extern int cli_register_net_ospf(SCliCmds * pCmds);
// ----- cli_register_net_subnet_ospf --------------------------------------
extern int cli_register_net_subnet_ospf(SCliCmds * pCmds);
// ----- cli_net_node_link_ospf_area ------------------------------------
extern int cli_net_node_link_ospf_area(SCliContext * pContext, STokens * pTokens);
// ----- cli_register_net_node_link_ospf --------------------------------
extern void cli_register_net_node_link_ospf(SCliCmds * pCmds);
#endif
#endif
