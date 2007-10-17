// ==================================================================
// @(#)export_ntf.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 15/10/07
// @lastdate 15/10/07
// ==================================================================

#ifndef __NET_EXPORT_NTF__
#define __NET_EXPORT_NTF__

#include <libgds/log.h>
#include <net/network.h>

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ net_export_ntf ]-----------------------------------------
  int net_export_ntf(SLogStream * pStream, SNetwork * pNetwork);

#ifdef __cplusplus
}
#endif

#endif /* __NET_EXPORT_NTF_H__ */
