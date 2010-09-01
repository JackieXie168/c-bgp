// ==================================================================
// @(#)net_domain.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 29/07/2005
// $Id: net_domain.h,v 1.5 2009-03-24 15:58:43 bqu Exp $
// ==================================================================

#ifndef __CLI_NET_DOMAIN_H__
#define __CLI_NET_DOMAIN_H__

#include <libgds/cli_ctx.h>

#ifdef __cplusplus
extern "C" {
#endif

  // ----- cli_net_add_domain ---------------------------------------
  int cli_net_add_domain(cli_ctx_t * ctx, cli_cmd_t * cmd);
  // ----- cli_register_net_domain ----------------------------------
  void cli_register_net_domain(cli_cmd_t * parent);

#ifdef __cplusplus
}
#endif

#endif /* __CLI_NET_DOMAIN_H__ */
