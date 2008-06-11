// ==================================================================
// @(#)ip_trace.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 01/03/2008
// $Id: ip_trace.h,v 1.5 2008-06-11 15:13:45 bqu Exp $
// ==================================================================

#ifndef __NET_IP_TRACE_H__
#define __NET_IP_TRACE_H__

#include <libgds/array.h>

#include <net/error.h>
#include <net/net_types.h>

typedef struct {
  net_elem_t    elt;       /* network element (node / subnet) */
  net_iface_t * iif;       /* incoming interface address */
  net_iface_t * oif;       /* outgoing interface address */
  void        * user_data; /* user-data (e.g. hop QoS data) */
} ip_trace_item_t;

typedef struct {
  SPtrArray        * items;    /* Traversed hops (nodes, subnets) */
  net_link_delay_t   delay;    /* QoS info: delay */
  igp_weight_t       weight;   /* QoS info: IGP weight */
  net_link_load_t    capacity; /* QoS info: max capacity */
  net_link_load_t    load;     /* Traffic volume to load */
  net_error_t        status;   /* General status of trace */
} ip_trace_t;

#define IP_TRACE_DUMP_LENGTH  0x01
#define IP_TRACE_DUMP_SUBNETS 0x02

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ ip_trace_create ]----------------------------------------
  ip_trace_t * ip_trace_create();
  // -----[ ip_trace_destroy ]---------------------------------------
  void ip_trace_destroy(ip_trace_t ** trace_ref);
  // -----[ ip_trace_add_node ]--------------------------------------
  ip_trace_item_t * ip_trace_add_node(ip_trace_t * trace,
				      net_node_t * node,
				      net_iface_t * iif,
				      net_iface_t * oif);
  // -----[ ip_trace_add_subnet ]------------------------------------
  ip_trace_item_t * ip_trace_add_subnet(ip_trace_t * trace,
					net_subnet_t * subnet);
  // -----[ ip_trace_search ]----------------------------------------
  int ip_trace_search(ip_trace_t * trace, net_node_t * node);
  // -----[ ip_trace_dump ]------------------------------------------
  void ip_trace_dump(SLogStream * stream, ip_trace_t * trace,
		     uint8_t options);

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

#endif /* __NET_IP_TRACE_H__ */
