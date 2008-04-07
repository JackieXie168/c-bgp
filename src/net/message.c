// ==================================================================
// @(#)message.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 23/02/2004
// @lastdate 18/04/2007
// ==================================================================

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <libgds/log.h>
#include <libgds/memory.h>

#include <net/message.h>
#include <net/protocol.h>

// ----- message_create ---------------------------------------------
/**
 *
 */
net_msg_t * message_create(net_addr_t src_addr, net_addr_t dst_addr,
			   uint8_t protocol, uint8_t ttl,
			   void * payload, FPayLoadDestroy destroy)
{
  net_msg_t * msg= (net_msg_t *) MALLOC(sizeof(net_msg_t));
  msg->src_addr= src_addr;
  msg->dst_addr= dst_addr;
  msg->protocol= protocol;
  msg->ttl= ttl;
  msg->tos= 0;
  msg->payload= payload;
  msg->ops.destroy= destroy;
  return msg;
}

// ----- message_destroy --------------------------------------------
/**
 *
 */
void message_destroy(net_msg_t ** msg_ref)
{
  if (*msg_ref != NULL) {
    if (((*msg_ref)->payload != NULL) &&
	((*msg_ref)->ops.destroy != NULL))
      (*msg_ref)->ops.destroy(&(*msg_ref)->payload);
    FREE(*msg_ref);
  }
}

// ----- message_dump -----------------------------------------------
/**
 *
 */
void message_dump(SLogStream * stream, net_msg_t * msg)
{
  log_printf(stream, "src:");
  ip_address_dump(stream, msg->src_addr);
  log_printf(stream, ", dst:");
  ip_address_dump(stream, msg->dst_addr);
  log_printf(stream, ", proto:%s, ttl:%d",
	     net_protocol2str(msg->protocol), msg->ttl);
}
