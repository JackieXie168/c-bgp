// ==================================================================
// @(#)net_domain.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 29/07/2005
// $Id: net_domain.h,v 1.4 2008-04-11 11:03:06 bqu Exp $
// ==================================================================

#ifndef __CLI_NET_DOMAIN_H__
#define __CLI_NET_DOMAIN_H__

#include <libgds/cli_ctx.h>

#ifdef __cplusplus
extern "C" {
#endif

  // ----- cli_net_add_domain ---------------------------------------
  int cli_net_add_domain(SCliContext * pContext, SCliCmd * pCmd);
  // ----- cli_register_net_domain ----------------------------------
  int cli_register_net_domain(SCliCmds * pCmds);

#ifdef __cplusplus
}
#endif

#endif /* __CLI_NET_DOMAIN_H__ */
