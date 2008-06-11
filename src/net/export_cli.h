// ==================================================================
// @(#)export_cli.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 15/10/07
// $Id: export_cli.h,v 1.3 2008-06-11 15:13:45 bqu Exp $
// ==================================================================

#ifndef __NET_EXPORT_CLI_H__
#define __NET_EXPORT_CLI_H__

#include <libgds/log.h>
#include <net/network.h>

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ net_export_cli ]-----------------------------------------
  int net_export_cli(SLogStream * pStream, network_t * network);

#ifdef __cplusplus
}
#endif

#endif /* __NET_EXPORT_CLI_H__ */
