// ==================================================================
// @(#)export_cli.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 15/10/07
// $Id: export_cli.h,v 1.2 2008-04-11 11:03:06 bqu Exp $
// ==================================================================

#ifndef __NET_EXPORT_CLI_H__
#define __NET_EXPORT_CLI_H__

#include <libgds/log.h>
#include <net/network.h>

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ net_export_cli ]-----------------------------------------
  int net_export_cli(SLogStream * pStream, SNetwork * pNetwork);

#ifdef __cplusplus
}
#endif

#endif /* __NET_EXPORT_CLI_H__ */
