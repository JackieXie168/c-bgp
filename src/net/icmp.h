// ==================================================================
// @(#)icmp.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 25/02/2004
// $Id: icmp.h,v 1.6 2008-04-11 11:03:06 bqu Exp $
// ==================================================================

#ifndef __NET_ICMP_H__
#define __NET_ICMP_H__

#include <net/ip_trace.h>
#include <net/prefix.h>
#include <net/message.h>
#include <net/network.h>
#include <net/net_path.h>
#include <net/icmp_options.h>

typedef enum {
  ICMP_ERROR_NET_UNREACH  = 1,
  ICMP_ERROR_HOST_UNREACH = 2,
  ICMP_ERROR_PROTO_UNREACH= 3,
  ICMP_ERROR_TIME_EXCEEDED= 4,
} icmp_error_type_t;

// ----- ICMP message types -----
typedef enum {
  ICMP_ECHO_REQUEST= 0,
  ICMP_ECHO_REPLY  = 1,
  ICMP_ERROR       = 2,
  ICMP_TRACE       = 3, /* This is a non-standard type used to perform
			 * "record-route" actions */
} icmp_msg_type_t;

typedef struct {
  icmp_msg_type_t         type;     /* ICMP type */
  icmp_error_type_t       sub_type; /* ICMP subtype (error) */
  net_node_t            * node;     /* Origin node */
  struct icmp_msg_opt_t   opts;     /* Options */
} icmp_msg_t;

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ icmp_send_error ]----------------------------------------
  int icmp_send_error(net_node_t * node, net_addr_t src_addr,
		      net_addr_t dst_addr, icmp_error_type_t tErrorCode,
		      simulator_t * sim);
  // -----[ icmp_send_echo_request ]---------------------------------
  int icmp_send_echo_request(net_node_t * node, net_addr_t src_addr,
			     net_addr_t dst_addr, uint8_t ttl,
			     simulator_t * sim);
  // -----[ icmp_event_handler ]-------------------------------------
  int icmp_event_handler(simulator_t * sim,
			 void * handler, net_msg_t * msg);
  // -----[ is_icmp_error ]------------------------------------------
  int is_icmp_error(net_msg_t * msg);
  // -----[ icmp_ping_send_recv ]------------------------------------
  int icmp_ping_send_recv(net_node_t * node, net_addr_t src_addr,
			  net_addr_t dst_addr, uint8_t ttl);
  
  ///////////////////////////////////////////////////////////////////
  // PROBES
  ///////////////////////////////////////////////////////////////////

  // -----[ icmp_ping ]----------------------------------------------
  int icmp_ping(SLogStream * stream,
		net_node_t * node, net_addr_t src_addr,
		net_addr_t dst_addr, uint8_t ttl);
  // -----[ icmp_trace_route ]---------------------------------------
  int icmp_trace_route(SLogStream * stream,
		       net_node_t * node, net_addr_t src_addr,
		       net_addr_t dst_addr, uint8_t max_ttl,
		       ip_trace_t ** trace_ref);
  // -----[ icmp_record_route ]--------------------------------------
  int icmp_record_route(net_node_t * node,
			net_addr_t dst_addr,
			ip_pfx_t * alt_dest,
			uint8_t max_ttl,
			uint8_t options,
			ip_trace_t ** trace_ref,
			net_link_load_t load);
  
#ifdef __cplusplus
}
#endif

#endif /* __NET_ICMP_H__ */
