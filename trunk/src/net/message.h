// ==================================================================
// @(#)message.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 23/02/2004
// $Id: message.h,v 1.6 2008-06-11 15:13:45 bqu Exp $
// ==================================================================

#ifndef __NET_MESSAGE_H__
#define __NET_MESSAGE_H__

#include <libgds/log.h>
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
  void message_destroy(net_msg_t ** ppMessage);
  // ----- message_dump ---------------------------------------------
  void message_dump(SLogStream * pStream, net_msg_t * pMessage);
  
#ifdef __cplusplus
}
#endif

#endif /* __NET_MESSAGE_H__ */
