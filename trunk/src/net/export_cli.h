// ==================================================================
// @(#)export_cli.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 15/10/07
// $Id: export_cli.h,v 1.4 2009-03-24 16:07:49 bqu Exp $
// ==================================================================

#ifndef __NET_EXPORT_CLI_H__
#define __NET_EXPORT_CLI_H__

#include <libgds/stream.h>
#include <net/network.h>

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ net_export_cli ]-----------------------------------------
  net_error_t net_export_cli(gds_stream_t * stream, network_t * network);

#ifdef __cplusplus
}
#endif

#endif /* __NET_EXPORT_CLI_H__ */
