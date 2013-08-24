// ==================================================================
// @(#)stats.h
//
// Network traffic statistics.
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 22/12/2008
// $Id: stats.h,v 1.1 2009-08-31 15:04:20 bqu Exp $
// ==================================================================

#ifndef __NET_TRAFFIC_STATS_H__
#define __NET_TRAFFIC_STATS_H__

#include <libgds/stream.h>

typedef struct flow_stats_t {
  unsigned int   flows_total;
  unsigned int   flows_ok;
  unsigned int   flows_error;
  unsigned int   bytes_total;
  unsigned int   bytes_ok;
  unsigned int   bytes_error;
} flow_stats_t;

#ifdef _cplusplus
extern "C" {
#endif

  /* !!! TO BE DOCUMENTED !!! */

  // -----[ flow_stats_init ]----------------------------------------
  void flow_stats_init(flow_stats_t * stats);

  // -----[ flow_stats_dump ]----------------------------------------
  void flow_stats_dump(gds_stream_t * stream, flow_stats_t * stats);

  // -----[ flow_stats_count ]---------------------------------------
  void flow_stats_count(flow_stats_t * stats, unsigned int bytes);

  // -----[ flow_stats_failure ]-------------------------------------
  void flow_stats_failure(flow_stats_t * stats, unsigned int bytes);

  // -----[ flow_stats_success ]-------------------------------------
  void flow_stats_success(flow_stats_t * stats, unsigned int bytes);

#ifdef _cplusplus
}
#endif

#endif /* __NET_TRAFFIC_STATS_H__ */
