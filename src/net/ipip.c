// ==================================================================
// @(#)ipip.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 25/02/2004
// $Id: ipip.c,v 1.7 2008-04-07 09:42:04 bqu Exp $
// ==================================================================

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>

#include <net/error.h>
#include <net/iface.h>
#include <net/ipip.h>
#include <net/link.h>
#include <net/network.h>
#include <net/node.h>
#include <net/protocol.h>

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

// -----[ ipip_iface_send ]------------------------------------------
/**
 *
 */
static int _ipip_iface_send(net_iface_t * self,
			    net_addr_t tNextHop,
			    net_msg_t * msg)
{
  ipip_data_t * ctx= (ipip_data_t *) self->user_data;
  net_addr_t src_addr= ctx->src_addr;

  /*log_printf(pLogErr, "send packet in tunnel (");
  message_dump(pLogErr, msg);
  log_printf(pLogErr, ") => src-addr:");
  ip_address_dump(pLogErr, src_addr);
  log_printf(pLogErr, " ; end-point:");
  ip_address_dump(pLogErr, self->dest.end_point);
  log_printf(pLogErr, "\n");*/

  // Send message through next interface

  if (ctx->oif != NULL) {
    // Default IP encap source address = outgoing interface's address.
    if (src_addr == NET_ADDR_ANY)
	src_addr= ctx->oif->tIfaceAddr;
    //return node_ip_output();
    return EUNSUPPORTED;
  } else {
    return node_send_msg(self->src_node,
			 src_addr,
			 self->dest.end_point,
			 NET_PROTOCOL_IPIP,
			 255, msg, _ipip_msg_destroy, NULL);
  }
}

// -----[ ipip_iface_recv ]------------------------------------------
/**
 * TODO: destroy payload.
 */
static int _ipip_iface_recv(net_iface_t * self, net_msg_t * msg)
{
  if (msg->protocol != NET_PROTOCOL_IPIP) {
    /* Discard packet silently ? should log */
    log_printf(pLogErr, "non-IPIP packet received on tunnel interface\n");
    return -1;
  }
  return node_recv_msg(self->src_node, self,
		       (net_msg_t *) msg->payload);
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
int ipip_send(net_node_t * pNode, net_addr_t tDstAddr,
	      net_msg_t * pMessage)
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
  SLogStream * syslog= node_syslog(node);

  /* Check that the receiving interface exists */
  iif= node_find_iface(node, net_prefix(msg->dst_addr, 32));
  if (iif == NULL) {
    log_printf(syslog, "IP-IP msg dst is not a local interface\n");
    return EUNSUPPORTED;
  }

  /* Check that the receiving interface is VIRTUAL */
  if (iif->type != NET_IFACE_VIRTUAL) {
    log_printf(syslog, "In node ");
    ip_address_dump(syslog, node->tAddr);
    log_printf(syslog, ",\nIP-IP message received but addressed "
	       "to wrong interface ");
    ip_address_dump(syslog, msg->dst_addr);
    log_printf(syslog, "\n");
    return EUNSUPPORTED;
  }

  /* Note: in the future, we shall check that the receiving interface
   * is IP-in-IP enabled */

  /* Deliver to node (forward if required ) */
  return node_recv_msg(node, iif, (net_msg_t *) msg->payload);
}
