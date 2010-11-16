// ==================================================================
// @(#)export_ntf.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 15/10/07
// $Id: export_ntf.h,v 1.3 2009-03-24 16:16:04 bqu Exp $
// ==================================================================

#ifndef __NET_EXPORT_NTF__
#define __NET_EXPORT_NTF__

#include <libgds/stream.h>
#include <net/network.h>

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ net_export_ntf ]-----------------------------------------
  int net_export_ntf(gds_stream_t * stream, network_t * network);

#ifdef __cplusplus
}
#endif

#endif /* __NET_EXPORT_NTF_H__ */
