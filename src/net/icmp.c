// ==================================================================
// @(#)icmp.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 25/02/2004
// $Id: icmp.c,v 1.12 2008-04-11 11:03:06 bqu Exp $
// ==================================================================

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <stdlib.h>

#include <net/error.h>
#include <net/icmp.h>
#include <net/node.h>
#include <net/prefix.h>
#include <net/protocol.h>
#include <net/network.h>
#include <net/net_types.h>

typedef struct {
  net_node_t * node;      // Node that received the message
  net_addr_t   src_addr;   // Source address of the message
  net_addr_t   dst_addr;   // Destination address of the message
  icmp_msg_t   msg;
  int          received;
} icmp_msg_info_t;
static icmp_msg_info_t tICMPRecv= { .received= 0 };


// -----[ icmp_perror ]----------------------------------------------
/**
 * Report comprehensive ICMP error description.
 */
void icmp_perror(SLogStream * pStream, icmp_error_type_t tErrorCode)
{
  switch (tErrorCode) {
  case ICMP_ERROR_NET_UNREACH:
    log_printf(pStream, "network unreachable"); break;
  case ICMP_ERROR_HOST_UNREACH:
    log_printf(pStream, "host unreachable"); break;
  case ICMP_ERROR_PROTO_UNREACH:
    log_printf(pStream, "protocol unreachable"); break;
  case ICMP_ERROR_TIME_EXCEEDED:
    log_printf(pStream, "time exceeded"); break;
  default:
    log_printf(pStream, "unknown error code (%d)", tErrorCode);
  }
}

// -----[ is_icmp_error ]--------------------------------------------
/**
 * Check if the message is an ICMP error. This is used in the node
 * forwarding function to avoid sending an ICMP error message when
 * we fail to forward/deliver another ICMP error message.
 */
int is_icmp_error(net_msg_t * msg)
{
  return ((msg->protocol != NET_PROTOCOL_ICMP) &&
	  (((icmp_msg_t *) msg->payload)->type == ICMP_ERROR));
}

// -----[ _icmp_msg_create ]-----------------------------------------
static inline icmp_msg_t * _icmp_msg_create(icmp_msg_type_t type,
					    icmp_error_type_t sub_type,
					    net_node_t * node)
{
  icmp_msg_t * icmp_msg= MALLOC(sizeof(icmp_msg_t));
  icmp_msg->type= type;
  icmp_msg->sub_type= sub_type;
  icmp_msg->node= node;
  icmp_msg->opts.trace= NULL;
  icmp_msg->opts.options= 0;
  return icmp_msg;
}

// -----[ _icmp_msg_destroy ]----------------------------------------
static inline void _icmp_msg_destroy(icmp_msg_t ** msg_ref)
{
  if (*msg_ref != NULL) {
    FREE(*msg_ref);
    *msg_ref= NULL;
  }
}

// -----[ icmp_send_error ]------------------------------------------
/**
 * Send an ICMP error message.
 */
int icmp_send_error(net_node_t * node,
		    net_addr_t src_addr,
		    net_addr_t dst_addr,
		    icmp_error_type_t tErrorCode,
		    simulator_t * sim)
{
  return node_send_msg(node, src_addr, dst_addr, NET_PROTOCOL_ICMP,
		       255, _icmp_msg_create(ICMP_ERROR, tErrorCode, node),
		       (FPayLoadDestroy) _icmp_msg_destroy,
		       sim);
}

// -----[ _icmp_send ]-----------------------------------------------
static inline int _icmp_send(net_node_t * node,
			     net_addr_t src_addr,
			     net_addr_t dst_addr,
			     uint8_t ttl,
			     icmp_msg_t * msg,
			     simulator_t * sim)
{
  return node_send_msg(node, src_addr, dst_addr, NET_PROTOCOL_ICMP,
		       ttl, msg,
		       (FPayLoadDestroy) _icmp_msg_destroy, sim);
}

// -----[ icmp_send_echo_request ]-----------------------------------
/**
 * Send an ICMP echo request to the given destination address.
 */
int icmp_send_echo_request(net_node_t * node,
			   net_addr_t src_addr,
			   net_addr_t dst_addr,
			   uint8_t ttl,
			   simulator_t * sim)
{
  return _icmp_send(node, src_addr, dst_addr, ttl,
		    _icmp_msg_create(ICMP_ECHO_REQUEST, 0, node),
		    sim);
}

// -----[ icmp_send_echo_reply ]-------------------------------------
/**
 * Send an ICMP echo reply to the given destination address. The
 * source address of the ICMP message is set to the given source
 * address. Depending on the implementation, the source address might
 * be the same as the destination address in the ICMP echo request
 * message instead of the address of the outgoing interface.
 */
int icmp_send_echo_reply(net_node_t * node,
			 net_addr_t src_addr,
			 net_addr_t dst_addr,
			 uint8_t ttl,
			 simulator_t * sim)
{
  return _icmp_send(node, src_addr, dst_addr, ttl,
		    _icmp_msg_create(ICMP_ECHO_REPLY, 0, node),
		    sim);
}

// -----[ _icmp_msg_dump ]-------------------------------------------
static inline void _icmp_msg_dump(net_node_t * node, net_msg_t * msg)
{
  icmp_msg_t * icmp_msg= (icmp_msg_t *) msg;

  ip_address_dump(pLogErr, node->addr);
  log_printf(pLogErr, "recv(");
  switch (icmp_msg->type) {
  case ICMP_ECHO_REQUEST:
    log_printf(pLogErr, "echo-request"); break;
  case ICMP_ECHO_REPLY:
    log_printf(pLogErr, "echo-reply"); break;
  case ICMP_TRACE:
    log_printf(pLogErr, "trace"); break;
  case ICMP_ERROR:
    log_printf(pLogErr, "error(%d)", icmp_msg->sub_type); break;
  }
  log_printf(pLogErr, "): src=");
  ip_address_dump(pLogErr, msg->src_addr);
  log_printf(pLogErr, ", dst=");
  ip_address_dump(pLogErr, msg->dst_addr);
  log_printf(pLogErr, "\n");
}

// -----[ icmp_event_handler ]---------------------------------------
/**
 * This function handles ICMP messages received by a node.
 */
int icmp_event_handler(simulator_t * sim,
		       void * handler,
		       net_msg_t * msg)
{
  net_node_t * node= (net_node_t *) handler;
  icmp_msg_t * icmp_msg= (icmp_msg_t *) msg->payload;

  // Record received ICMP messages. This is used by the icmp_ping()
  // and icmp_traceroute() functions to check for replies.
  tICMPRecv.received= 1;
  tICMPRecv.node= node;
  tICMPRecv.src_addr= msg->src_addr;
  tICMPRecv.dst_addr= msg->dst_addr;
  tICMPRecv.msg= *icmp_msg;

  switch (tICMPRecv.msg.type) {
  case ICMP_ECHO_REQUEST:
    icmp_send_echo_reply(node, msg->dst_addr, msg->src_addr, 255, sim);
    break;

  case ICMP_TRACE:
    icmp_msg->opts.trace->status= ESUCCESS;
    break;

  case ICMP_ECHO_REPLY:
  case ICMP_ERROR:
    break;

  default:
    LOG_ERR(LOG_LEVEL_FATAL, "Error: unsupported ICMP message type (%d)\n",
	    tICMPRecv.msg.type);
    abort();
  }

  _icmp_msg_destroy(&icmp_msg);

  return 0;
}

// -----[ icmp_ping_send_recv ]--------------------------------------
/**
 * This function sends an ICMP echo-request and checks that an answer
 * is received. It can be used to silently check that a remote host
 * is reachable (e.g. in BGP session establishment and refresh
 * checks).
 *
 * For this purpose, the function instanciates a local simulator that
 * is used to schedule the messages exchanged in reaction to the ICMP
 * echo-request.
 */
net_error_t icmp_ping_send_recv(net_node_t * node, net_addr_t src_addr,
				net_addr_t dst_addr, uint8_t ttl)
{
  simulator_t * sim= NULL;
  net_error_t error;

  sim= sim_create(SCHEDULER_STATIC);

  tICMPRecv.received= 0;
  error= icmp_send_echo_request(node, src_addr, dst_addr, ttl, sim);
  if (error == ESUCCESS) {
    sim_run(sim);

    if ((tICMPRecv.received) && (tICMPRecv.node == node)) {
      switch (tICMPRecv.msg.type) {
      case ICMP_ECHO_REPLY:
	error= ESUCCESS;
	break;
      case ICMP_ERROR:
	switch (tICMPRecv.msg.sub_type) {
	case ICMP_ERROR_NET_UNREACH:
	  error= ENET_ICMP_NET_UNREACH; break;
	case ICMP_ERROR_HOST_UNREACH:
	  error= ENET_ICMP_HOST_UNREACH; break;
	case ICMP_ERROR_PROTO_UNREACH:
	  error= ENET_ICMP_PROTO_UNREACH; break;
	case ICMP_ERROR_TIME_EXCEEDED:
	  error= ENET_ICMP_TIME_EXCEEDED; break;
	default:
	  abort();
	}
	break;
      default:
	error= ENET_NO_REPLY;
      }
    } else {
      error= ENET_NO_REPLY;
    }
  }

  sim_destroy(&sim);
  return error;
}

// -----[ icmp_ping ]------------------------------------------------
/**
 * Send an ICMP echo-request to the given destination and checks if a
 * reply is received. This function mimics the well-known 'ping'
 * application.
 */
int icmp_ping(SLogStream * stream,
	      net_node_t * node, net_addr_t src_addr,
	      net_addr_t dst_addr, uint8_t ttl)
{
  int iResult;

  if (ttl == 0)
    ttl= 255;

  iResult= icmp_ping_send_recv(node, src_addr, dst_addr, ttl);

  log_printf(stream, "ping: ");
  switch (iResult) {
  case ESUCCESS:
    log_printf(stream, "reply from ");
    ip_address_dump(stream, tICMPRecv.src_addr);
    break;
  case ENET_ICMP_NET_UNREACH:
  case ENET_ICMP_HOST_UNREACH:
  case ENET_ICMP_PROTO_UNREACH:
  case ENET_ICMP_TIME_EXCEEDED:
    network_perror(stream, iResult);
    log_printf(stream, " from ");
    ip_address_dump(stream, tICMPRecv.src_addr);
    break;
  default:
    network_perror(stream, iResult);
  }
  log_printf(stream, "\n");
  log_flush(stream);

  return iResult;
}

// -----[ icmp_trace_route ]-----------------------------------------
/**
 * Send an ICMP echo-request with increasing TTL to the given
 * destination and checks if a 'time-exceeded' error or an echo-reply
 * is received. This function mimics the well-known 'traceroute'
 * application.
 */
net_error_t icmp_trace_route(SLogStream * stream,
			     net_node_t * node, net_addr_t src_addr,
			     net_addr_t dst_addr, uint8_t max_ttl,
			     ip_trace_t ** trace_ref)
{
  ip_trace_t * trace= ip_trace_create();
  net_error_t error= EUNEXPECTED;
  uint8_t ttl= 1;

  if (max_ttl == 0)
    max_ttl= 255;

  ip_trace_add_node(trace, node, NET_ADDR_ANY, NET_ADDR_ANY);

  while (ttl <= max_ttl) {

    error= icmp_ping_send_recv(node, src_addr, dst_addr, ttl);

    if (stream != NULL)
      log_printf(stream, "%3d\t", ttl);
    switch (error) {
    case ESUCCESS:
    case ENET_ICMP_NET_UNREACH:
    case ENET_ICMP_HOST_UNREACH:
    case ENET_ICMP_PROTO_UNREACH:
    case ENET_ICMP_TIME_EXCEEDED:
      if (stream != NULL) {
	ip_address_dump(stream, tICMPRecv.src_addr);
	log_printf(stream, " (");
	ip_address_dump(stream, tICMPRecv.msg.node->addr);
	log_printf(stream, ")");
      }
      ip_trace_add_node(trace, tICMPRecv.msg.node,
			tICMPRecv.src_addr,
			NET_ADDR_ANY);
      break;
    default:
      if (stream != NULL)
	log_printf(stream, "* (*)");
      ip_trace_add_node(trace, NULL, NET_ADDR_ANY, NET_ADDR_ANY);
    }

    if (stream != NULL) {
      log_printf(stream, "\t");
      if (error == ESUCCESS)
	log_printf(stream, "reply");
      else
	network_perror(stream, error);
      log_printf(stream, "\n");
    }
    
    if (error != ENET_ICMP_TIME_EXCEEDED)
      break;
      
    ttl++;
  }

  if (stream != NULL)
    log_flush(stream);

  if (trace_ref != NULL)
    *trace_ref= trace;
  else
    ip_trace_destroy(&trace);

  return error;
}

// -----[ icmp_record_route ]----------------------------------------
int icmp_record_route(net_node_t * node,
		      net_addr_t dst_addr,
		      ip_pfx_t * alt_dest,
		      uint8_t max_ttl,
		      uint8_t options,
		      ip_trace_t ** trace_ref,
		      net_link_load_t load)
{
  simulator_t * sim= sim_create(SCHEDULER_STATIC);
  icmp_msg_t * icmp_msg= _icmp_msg_create(ICMP_TRACE, 0, node);
  ip_trace_t * trace= ip_trace_create();
  net_error_t error;
  trace->status= ENET_HOST_UNREACH;
  trace->load= load;
  icmp_msg->opts.trace= trace;
  icmp_msg->opts.options= options;

  if (options & ICMP_RR_OPTION_ALT_DEST)
    icmp_msg->opts.alt_dest= *alt_dest;

  // Send ICMP record-route probe
  error= _icmp_send(node, NET_ADDR_ANY, dst_addr, max_ttl, icmp_msg, sim);
  sim_run(sim);
  sim_destroy(&sim);

  if (trace_ref != NULL)
    *trace_ref= trace;
  else
    ip_trace_destroy(&trace);

  return error;
}
