// ==================================================================
// @(#)ipip.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 25/02/2004
// $Id: ipip.c,v 1.10 2009-03-24 16:14:28 bqu Exp $
// ==================================================================

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>

#include <net/error.h>
#include <net/icmp_options.h>
#include <net/iface.h>
#include <net/ipip.h>
#include <net/link.h>
#include <net/network.h>
#include <net/node.h>
#include <net/protocol.h>
#include <util/str_format.h>

//#define IPIP_DEBUG

#ifdef IPIP_DEBUG
static int _debug_for_each(gds_stream_t * stream, void * context,
			   char format)
{
  va_list * ap= (va_list*) context;
  net_node_t * node;
  net_addr_t addr;
  net_msg_t * msg;
  net_iface_t * iface;
  int error;

  switch (format) {
  case 'a':
    addr= va_arg(*ap, net_addr_t);
    ip_address_dump(stream, addr);
    break;
  case 'e':
    error= va_arg(*ap, int);
    network_perror(stream, error);
    break;
  case 'i':
    iface= va_arg(*ap, net_iface_t *);
    net_iface_dump_id(stream, iface);
    break;
  case 'm':
    msg= va_arg(*ap, net_msg_t *);
    stream_printf(stream, "[");
    message_dump(stream, msg);
    stream_printf(stream, "]");
    break;
  case 'n':
    node= va_arg(*ap, net_node_t *);
    node_dump_id(stream, node);
    break;
  }
  return 0;
}
#endif /* IPIP_DEBUG */

static inline void ___ipip_debug(const char * msg, ...)
{
#ifdef IPIP_DEBUG
  va_list ap;

  va_start(ap, msg);
  stream_printf(gdsout, "IPIP_DBG::");
  str_format_for_each(gdsout, _debug_for_each, &ap, msg);
  stream_flush(gdsout);
  va_end(ap);
#endif /* IPIP_DEBUG */
}

typedef struct {
  net_iface_t * oif;      /* iface used to send traffic (by-pass routing) */
  net_addr_t    gateway;
  net_addr_t    src_addr;
} ipip_data_t;

// -----[ _ipip_link_destroy ]---------------------------------------
static void _ipip_link_destroy(void * ctx)
{
  FREE(ctx);
}

// -----[ _ipip_msg_destroy ]----------------------------------------
/**
 *
 */
static void _ipip_msg_destroy(void ** payload_ref)
{
  //net_msg_t * msg= *((net_msg_t **) payload_ref);
  //message_destroy(&msg);
}

// -----[ _ipip_proto_dump_msg ]-------------------------------------
static void _ipip_proto_dump_msg(gds_stream_t * stream, net_msg_t * msg)
{
  net_msg_t * encap_msg= (net_msg_t *) msg->payload;
  stream_printf(stream, "msg:[");
  message_dump(stream, encap_msg);
  stream_printf(stream, "]");
}

// -----[ _ipip_proto_copy_payload ]---------------------------------
static void * _ipip_proto_copy_payload(net_msg_t * msg)
{
  net_msg_t * inner_msg= message_copy((net_msg_t *) msg->payload);
  return inner_msg;
}

// -----[ ipip_iface_send ]------------------------------------------
/**
 *
 */
static int _ipip_iface_send(net_iface_t * self,
			    net_addr_t next_hop,
			    net_msg_t * msg)
{
  ipip_data_t * ctx= (ipip_data_t *) self->user_data;
  net_addr_t src_addr= ctx->src_addr;
  net_msg_t * outer_msg;

  ___ipip_debug("send msg=%m\n", msg);

  if (ctx->oif != NULL) {
    // Default IP encap source address = outgoing interface's address.
    if (src_addr == NET_ADDR_ANY)
	src_addr= ctx->oif->addr;

    //TO BE WRITTEN: return node_ip_output(); ...
    return EUNSUPPORTED;

  } else {

    outer_msg= message_create(src_addr, self->dest.end_point,
			      NET_PROTOCOL_IPIP, 255, msg,
			      _ipip_msg_destroy);
    if (msg->opts != NULL) {
      outer_msg->opts= ip_options_copy(msg->opts);
      outer_msg->opts->flags&= ~IP_OPT_ALT_DEST;
    }
    ip_opt_hook_msg_encap(self->owner, outer_msg, msg);

    return node_send(self->owner, outer_msg,
		     NULL, NULL);
  }
}

// -----[ ipip_iface_recv ]------------------------------------------
/**
 * TODO: destroy payload.
 */
static int _ipip_iface_recv(net_iface_t * self, net_msg_t * msg)
{
  ___ipip_debug("rcvd msg=%m\n", msg);

  if (msg->protocol != NET_PROTOCOL_IPIP) {
    /* Discard packet silently ? should log */
    stream_printf(gdserr, "non-IPIP packet received on tunnel interface\n");
    return -1;
  }

  ip_opt_hook_msg_decap(self->owner, msg, (net_msg_t *) msg->payload);

  return node_recv_msg(self->owner, self, (net_msg_t *) msg->payload);
}

// -----[ ipip_link_create ]-----------------------------------------
/**
 * Create a tunnel interface.
 *
 * Arguments:
 *   node     : source node (where the interface is attached.
 *   dst-point: remote tunnel endpoint
 *   addr     : tunnel identifier
 *   oif      : optional outgoing interface
 *   src_addr : optional IPIP packet source address
 */
net_error_t ipip_link_create(net_node_t * node,
			     net_addr_t end_point,
			     net_addr_t addr,
			     net_iface_t * oif,
			     net_addr_t src_addr,
			     net_iface_t ** ppLink)
{
  net_iface_t * tunnel;
  ipip_data_t * ctx;
  net_error_t error;

  // Check that the source address corresponds to a local interface
  if (src_addr != NET_ADDR_ANY) {
    if (node_find_iface(node, net_prefix(src_addr, 32)) == NULL) {
      abort();
    }
  }

  // Create the tunnel interface
  error= net_iface_factory(node, net_prefix(addr, 32), NET_IFACE_VIRTUAL,
			   &tunnel);
  if (error != ESUCCESS)
    return error;

  // Connect tunnel interface
  tunnel->dest.end_point= end_point;
  tunnel->connected= 1;

  // Create the IPIP context
  ctx= (ipip_data_t *) MALLOC(sizeof(ipip_data_t));
  ctx->oif= oif;
  ctx->gateway= NET_ADDR_ANY;
  ctx->src_addr= src_addr;

  tunnel->user_data= ctx;
  tunnel->ops.send= _ipip_iface_send;
  tunnel->ops.recv= _ipip_iface_recv;
  tunnel->ops.destroy= _ipip_link_destroy;

  *ppLink= tunnel;

  return ESUCCESS;
}

// ----- ipip_send --------------------------------------------------
/**
 * Encapsulate the given message and sent it to the given tunnel
 * endpoint address.
 */
int ipip_send(net_node_t * node, net_addr_t dst_addr,
	      net_msg_t * msg)
{
  return ESUCCESS;
}

// -----[ ipip_event_handler ]---------------------------------------
/**
 * IP-in-IP protocol handler. Decapsulate IP-in-IP messages and send
 * to encapsulated message's destination.
 */
int ipip_event_handler(simulator_t * sim,
		       void * handler,
		       net_msg_t * msg)
{
  net_node_t * node= (net_node_t *) handler;
  net_iface_t * iif= NULL;
  gds_stream_t * syslog= node_syslog(node);

  /*
  stream_printf(gdserr, "IPIP msg handling {");
  message_dump(gdserr, msg);
  stream_printf(gdserr, "}\n");
*/

  /* Check that the receiving interface exists */
  iif= node_find_iface(node, net_prefix(msg->dst_addr, 32));
  if (iif == NULL) {
    stream_printf(syslog, "IP-IP msg dst is not a local interface\n");
    return EUNSUPPORTED;
  }

  /* Check that the receiving interface is VIRTUAL */
  if (iif->type != NET_IFACE_VIRTUAL) {
    stream_printf(syslog, "In node ");
    ip_address_dump(syslog, node->rid);
    stream_printf(syslog, ",\nIP-IP message received but addressed "
	       "to wrong interface ");
    ip_address_dump(syslog, msg->dst_addr);
    stream_printf(syslog, "\n");
    return EUNSUPPORTED;
  }

  /* Note: in the future, we shall check that the receiving interface
   * is IP-in-IP enabled */

  /* Deliver to node (forward if required ) */
  return node_recv_msg(node, iif, (net_msg_t *) msg->payload);
}

const net_protocol_def_t PROTOCOL_IPIP= {
  .name= "ipip",
  .ops= {
    .handle      = NULL,
    .destroy     = NULL,
    .dump_msg    = _ipip_proto_dump_msg,
    .destroy_msg = NULL,
    .copy_payload= _ipip_proto_copy_payload,
  }
};
