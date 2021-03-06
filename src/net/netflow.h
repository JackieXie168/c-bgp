// ==================================================================
// @(#)netflow.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 14/05/2007
// $Id: netflow.h,v 1.4 2009-08-31 09:48:28 bqu Exp $
// ==================================================================

#ifndef __NET_NETFLOW_H__
#define __NET_NETFLOW_H__

#include <stdio.h>

#include <libgds/stream.h>

#include <net/prefix.h>
#include <net/tm.h>
#include <util/lrp.h>

// -----[ netflow_error_t ]------------------------------------------
typedef enum {
  NETFLOW_SUCCESS               = LRP_SUCCESS,
  NETFLOW_ERROR                 = LRP_ERROR_USER,
  NETFLOW_ERROR_OPEN            = LRP_ERROR_USER-1,
  NETFLOW_ERROR_NUM_FIELDS      = LRP_ERROR_USER-2,
  NETFLOW_ERROR_INVALID_HEADER  = LRP_ERROR_USER-3,
  NETFLOW_ERROR_INVALID_SRC_ADDR= LRP_ERROR_USER-4,
  NETFLOW_ERROR_INVALID_DST_ADDR= LRP_ERROR_USER-5,
  NETFLOW_ERROR_INVALID_SRC_ASN = LRP_ERROR_USER-6,
  NETFLOW_ERROR_INVALID_DST_ASN = LRP_ERROR_USER-7,
  NETFLOW_ERROR_INVALID_DST_MASK= LRP_ERROR_USER-8,
  NETFLOW_ERROR_INVALID_OCTETS  = LRP_ERROR_USER-9,
  NETFLOW_ERROR_MISSING_FIELD   = LRP_ERROR_USER-10,
} netflow_error_t;

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ netflow_perror ]-----------------------------------------
  void netflow_perror(gds_stream_t * stream, int error);
  // -----[ netflow_strerror ]---------------------------------------
  const char * netflow_strerror(int error);
  // -----[ netflow_load ]-------------------------------------------
  int netflow_load(const char * filename, flow_field_map_t * map,
		   flow_handler_f handler, void * ctx);

  // -----[ _netflow_init ]------------------------------------------
  void _netflow_init();
  // -----[ _netflow_done ]------------------------------------------
  void _netflow_done();

#ifdef __cplusplus
}
#endif

#endif /* __NET_NETFLOW_H__ */
