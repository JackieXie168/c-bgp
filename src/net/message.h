// ==================================================================
// @(#)message.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 23/02/2004
// @lastdate 22/01/2007
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
  net_msg_t * message_create(net_addr_t tSrcAddr, net_addr_t tDstAddr,
			     uint8_t uProtocol, uint8_t uTTL,
			     void * pPayLoad,
			     FPayLoadDestroy fDestroy);
  // ----- message_destroy ------------------------------------------
  void message_destroy(net_msg_t ** ppMessage);
  // ----- message_dump ---------------------------------------------
  void message_dump(SLogStream * pStream, net_msg_t * pMessage);
  
#ifdef __cplusplus
}
#endif

#endif /* __NET_MESSAGE_H__ */
