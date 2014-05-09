// ==================================================================
// @(#)ip_trace.h
//
// @author Bruno Quoitin (bruno.quoitin@umons.ac.be)
// @date 01/03/2008
// $Id: ip_trace.h,v 1.6 2009-03-24 16:13:52 bqu Exp $
// ==================================================================

/**
 * \file
 * Provide data structures and functions to handle IP traces. The
 * IP traces can contain nodes, subnets and sub-traces. Each hop
 * can also specify an incoming and an outgoing network interface.
 * This allows to follow the exact IP path when nodes have multiple
 * possible outgoing and incoming interfaces.
 */

#ifndef __NET_IP_TRACE_H__
#define __NET_IP_TRACE_H__

#include <libgds/array.h>

#include <net/error.h>
#include <net/net_types.h>


// -----[ ip_trace_item_t ]------------------------------------------
/** Definition of an IP trace element (node/subnet/trace). */
typedef struct {
  /** Network element (node / subnet / trace / ...) */
  net_elem_t    elt;
  /** Incoming network interface. */
  net_iface_t * iif;
  /** Outgoing network interface. */
  net_iface_t * oif;
  /** User-data (e.g. hop QoS data). */
  void        * user_data;
} ip_trace_item_t;

#define IP_TRACE_DUMP_LENGTH  0x01
#define IP_TRACE_DUMP_SUBNETS 0x02


#ifdef __cplusplus
extern "C" {
#endif

  // -----[ ip_trace_create ]----------------------------------------
  /**
   * Create an IP trace.
   */
  ip_trace_t * ip_trace_create();
  
  // -----[ ip_trace_destroy ]---------------------------------------
  /**
   * Destroy an IP trace.
   */
  void ip_trace_destroy(ip_trace_t ** trace_ref);

  // -----[ ip_trace_add_node ]--------------------------------------
  ip_trace_item_t * ip_trace_add_node(ip_trace_t * trace,
				      net_node_t * node,
				      net_iface_t * iif,
				      net_iface_t * oif);
  // -----[ ip_trace_add_subnet ]------------------------------------
  ip_trace_item_t * ip_trace_add_subnet(ip_trace_t * trace,
					net_subnet_t * subnet);
  // -----[ ip_trace_add_trace ]-------------------------------------
  ip_trace_item_t * ip_trace_add_trace(ip_trace_t * trace,
				       ip_trace_t * trace_item);
  // -----[ ip_trace_search ]----------------------------------------
  int ip_trace_search(ip_trace_t * trace, net_node_t * node);
  // -----[ ip_trace_dump ]------------------------------------------
  void ip_trace_dump(gds_stream_t * stream, ip_trace_t * trace,
		     uint8_t options);
  // -----[ ip_trace_copy ]------------------------------------------
  ip_trace_t * ip_trace_copy(ip_trace_t * trace);

#ifdef __cplusplus
}
#endif

// -----[ ip_trace_length ]------------------------------------------
static inline unsigned int ip_trace_length(ip_trace_t * trace) {
  return ptr_array_length(trace->items);
}

// -----[ ip_trace_item_at ]-------------------------------------------
static inline ip_trace_item_t * ip_trace_item_at(ip_trace_t * trace,
						 unsigned int index) {
  return trace->items->data[index];
}

// -----[ ip_trace_last_item ]---------------------------------------
static inline ip_trace_item_t * ip_trace_last_item(ip_trace_t * trace) {
  unsigned int len= ip_trace_length(trace);
  if (len <= 0)
    return NULL;
  return trace->items->data[len-1];
}

#endif /* __NET_IP_TRACE_H__ */
