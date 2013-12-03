// ==================================================================
// @(#)export_graphviz.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 01/09/08
// $Id$
// ==================================================================

#ifndef __NET_EXPORT_GRAPHVIZ_H__
#define __NET_EXPORT_GRAPHVIZ_H__

#include <libgds/stream.h>
#include <net/net_types.h>

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ net_export_dot ]----------------------------------------
  net_error_t net_export_dot(gds_stream_t * stream, network_t * network);

#ifdef __cplusplus
}
#endif

#endif /* __NET_EXPORT_GRAPHVIZ_H__ */
