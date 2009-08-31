// ==================================================================
// @(#)message.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 23/02/2004
// $Id: message.c,v 1.10 2009-08-31 09:48:28 bqu Exp $
// ==================================================================

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <libgds/stream.h>
#include <libgds/memory.h>

#include <net/icmp_options.h>
#include <net/message.h>
#include <net/protocol.h>

// -----[ message_create ]-------------------------------------------
net_msg_t * message_create(net_addr_t src_addr, net_addr_t dst_addr,
			   net_protocol_id_t proto, uint8_t ttl,
			   void * payload, FPayLoadDestroy destroy)
{
  net_msg_t * msg= (net_msg_t *) MALLOC(sizeof(net_msg_t));
  msg->src_addr= src_addr;
  msg->dst_addr= dst_addr;
  msg->protocol= proto;
  msg->ttl= ttl;
  msg->tos= 0;
  msg->payload= payload;
  msg->ops.destroy= destroy;
  msg->opts= NULL;
  return msg;
}

// -----[ message_destroy ]------------------------------------------
void message_destroy(net_msg_t ** msg_ref)
{
  net_msg_t * msg= *msg_ref;

  if (msg != NULL) {
    if ((msg->payload != NULL) &&
	(msg->ops.destroy != NULL))
      msg->ops.destroy(&msg->payload);
    if (msg->opts != NULL)
      ip_options_destroy(&msg->opts);
    FREE(msg);
    *msg_ref= NULL;
  }
}

// -----[ message_set_options ]--------------------------------------
void message_set_options(net_msg_t * msg, struct ip_opt_t * opts)
{
  if (msg->opts != NULL)
    ip_options_destroy(&msg->opts);
  msg->opts= opts;
  if (opts != NULL)
    ip_options_add_ref(opts);
}

// -----[ message_copy ]---------------------------------------------
net_msg_t * message_copy(net_msg_t * msg)
{
  net_msg_t * new_msg;
  const net_protocol_def_t * proto_def= PROTOCOL_DEFS[msg->protocol];
  
  if (proto_def->ops.copy_payload == NULL)
    cbgp_fatal("NO COPY FOR PROTOCOL %d\n", msg->protocol);

  new_msg= message_create(msg->src_addr,
			  msg->dst_addr,
			  msg->protocol,
			  msg->ttl,
			  proto_def->ops.copy_payload(msg),
			  msg->ops.destroy);
  if (msg->opts != NULL)
    new_msg->opts= ip_options_copy(msg->opts);
  return new_msg;
}

// -----[ message_dump ]---------------------------------------------
void message_dump(gds_stream_t * stream, net_msg_t * msg)
{
  const net_protocol_def_t * proto_def;
  stream_printf(stream, "src:");
  ip_address_dump(stream, msg->src_addr);
  stream_printf(stream, ", dst:");
  ip_address_dump(stream, msg->dst_addr);
  stream_printf(stream, ", proto:%s, ttl:%d",
	     net_protocol2str(msg->protocol), msg->ttl);
  stream_printf(stream, ", payload:", msg->payload);
  proto_def= net_protocols_get_def(msg->protocol);
  if (proto_def->ops.dump_msg != NULL)
    proto_def->ops.dump_msg(stream, msg);
  else
    stream_printf(stream, "opaque");
}
