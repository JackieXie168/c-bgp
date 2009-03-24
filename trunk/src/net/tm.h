// ==================================================================
// @(#)tm.h
//
// Traffic matrix management.
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 23/01/2007
// $Id: tm.h,v 1.4 2009-03-24 16:28:11 bqu Exp $
// ==================================================================

/**
 * \file
 * Provide data structures and functions to parse and load a traffic
 * matrix out of a text file.
 *
 * The traffic matric file format is defined as follows. Each line
 * defines a traffic flow that enters the network through a node. The
 * flow enters the node through a specified network interface
 * identified by its IP address. The flow is destined to a
 * destination IP address or IP prefix. The flow record also defines
 * the flow volume in bytes (or whatever makes sense for the user).
 *
 * Each line contains 4 fields separated by a space or tabluation:
 *
 *   <node-id> <in-iface> <dst> <volume>
 *
 * where
 * \li <node-id> is the node's identifier (an IP address).
 * \li <in-iface> is the inbound network interface identifier
 *   (an IP address)
 * \li <dst is the flow destination (IP address or IP prefix).
 * \li <volume> is the flow volume.
 */

#ifndef __NET_TM_H__
#define __NET_TM_H__

#include <libgds/stream.h>

#include <util/lrp.h>

// -----[ net_traffic_handler_f ]------------------------------------
typedef int (*net_traffic_handler_f)(net_addr_t src, net_addr_t dst,
				     unsigned int bytes, void * ctx);

// -----[ tm_error_t ]-----------------------------------------------
typedef enum {
  NET_TM_SUCCESS           = LRP_SUCCESS,
  NET_TM_ERROR             = LRP_ERROR_USER,
  NET_TM_ERROR_OPEN        = LRP_ERROR_USER-1,
  NET_TM_ERROR_NUM_PARAMS  = LRP_ERROR_USER-2,
  NET_TM_ERROR_INVALID_SRC = LRP_ERROR_USER-3,
  NET_TM_ERROR_UNKNOWN_SRC = LRP_ERROR_USER-4,
  NET_TM_ERROR_INVALID_DST = LRP_ERROR_USER-5,
  NET_TM_ERROR_INVALID_LOAD= LRP_ERROR_USER-6,
} tm_error_t;

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ net_tm_strerror ]----------------------------------------
  char * net_tm_strerror(int error);
  // -----[ net_tm_perror ]------------------------------------------
  void net_tm_perror(gds_stream_t * stream, int error);
  // -----[ net_tm_parser ]------------------------------------------
  int net_tm_parser(FILE * stream);
  // -----[ net_tm_load ]--------------------------------------------
  int net_tm_load(const char * filename);

  // -----[ _tm_init ]-----------------------------------------------
  void _tm_init();
  // -----[ _tm_done ]-----------------------------------------------
  void _tm_done();

#ifdef __cplusplus
}
#endif

#endif /* __NET_TM_H__ */
