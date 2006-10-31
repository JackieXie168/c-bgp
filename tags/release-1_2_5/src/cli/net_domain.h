// ==================================================================
// @(#)net_domain.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 29/07/2005
// @lastdate 29/07/2005
// ==================================================================

#ifndef __CLI_NET_DOMAIN_H__
#define __CLI_NET_DOMAIN_H__

#include <libgds/cli_ctx.h>

// ----- cli_net_add_domain -----------------------------------------
extern int cli_net_add_domain(SCliContext * pContext, STokens * pTokens);
// ----- cli_net_node_domain ----------------------------------------
extern int cli_net_node_domain(SCliContext * pContext, STokens * pTokens);
// ----- cli_register_net_domain ------------------------------------
extern int cli_register_net_domain(SCliCmds * pCmds);

#endif
