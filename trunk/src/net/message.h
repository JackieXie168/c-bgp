// ==================================================================
// @(#)message.h
//
// @author Bruno Quoitin (bruno.quoitin@umons.ac.be)
// @date 23/02/2004
// $Id: message.h,v 1.8 2009-03-24 16:17:40 bqu Exp $
// ==================================================================

#ifndef __NET_MESSAGE_H__
#define __NET_MESSAGE_H__

#include <libgds/stream.h>
#include <net/prefix.h>
#include <net/net_types.h>

#ifdef __cplusplus
extern "C" {
#endif

  // ----- message_create -------------------------------------------
  net_msg_t * message_create(net_addr_t src_addr, net_addr_t dst_addr,
			     net_protocol_id_t proto, uint8_t ttl,
			     void * payload,
			     FPayLoadDestroy destroy);
  // ----- message_destroy ------------------------------------------
  void message_destroy(net_msg_t ** msg_ref);
  // -----[ message_set_options ]------------------------------------
  void message_set_options(net_msg_t * msg, struct ip_opt_t * opts);
  // -----[ message_copy ]-------------------------------------------
  net_msg_t * message_copy(net_msg_t * msg);
  // ----- message_dump ---------------------------------------------
  void message_dump(gds_stream_t * stream, net_msg_t * msg);
  
#ifdef __cplusplus
}
#endif

#endif /* __NET_MESSAGE_H__ */
