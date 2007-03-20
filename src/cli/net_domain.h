// ==================================================================
// @(#)net_domain.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 29/07/2005
// @lastdate 16/01/2007
// ==================================================================

#ifndef __CLI_NET_DOMAIN_H__
#define __CLI_NET_DOMAIN_H__

#include <libgds/cli_ctx.h>

#ifdef __cplusplus
extern "C" {
#endif

  // ----- cli_net_add_domain ---------------------------------------
  int cli_net_add_domain(SCliContext * pContext, SCliCmd * pCmd);
  // ----- cli_net_node_domain --------------------------------------
  int cli_net_node_domain(SCliContext * pContext, SCliCmd * pCmd);
  // ----- cli_register_net_domain ----------------------------------
  int cli_register_net_domain(SCliCmds * pCmds);

#ifdef __cplusplus
}
#endif

#endif /* __CLI_NET_DOMAIN_H__ */
