// ==================================================================
// @(#)export_ntf.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 15/10/07
// $Id: export_ntf.h,v 1.2 2008-06-11 15:13:45 bqu Exp $
// ==================================================================

#ifndef __NET_EXPORT_NTF__
#define __NET_EXPORT_NTF__

#include <libgds/log.h>
#include <net/network.h>

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ net_export_ntf ]-----------------------------------------
  int net_export_ntf(SLogStream * pStream, network_t * network);

#ifdef __cplusplus
}
#endif

#endif /* __NET_EXPORT_NTF_H__ */
