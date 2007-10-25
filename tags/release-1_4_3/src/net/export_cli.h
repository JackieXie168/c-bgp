// ==================================================================
// @(#)export_cli.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 15/10/07
// @lastdate 15/10/07
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
