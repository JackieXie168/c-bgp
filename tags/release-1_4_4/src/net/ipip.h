// ==================================================================
// @(#)ipip.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 25/02/2004
// @lastdate 16/04/2007
// ==================================================================

#ifndef __NET_IPIP_H__
#define __NET_IPIP_H__

#include <net/prefix.h>
#include <net/message.h>
#include <net/network.h>

#ifdef __cplusplus
extern "C" {
#endif

  // ----- ipip_send ------------------------------------------------
  int ipip_send(SNetNode * pNode, net_addr_t tDstAddr,
		SNetMessage * pMessage);
  // ----- ipip_event_handler ---------------------------------------
  int ipip_event_handler(SSimulator * pSimulator,
			 void * pHandler, SNetMessage * pMessage);
  // -----[ ipip_link_create ]---------------------------------------
  int ipip_link_create(SNetNode * pNode, net_addr_t tDstPoint,
		       net_addr_t tAddr, SNetLink * pOutIface,
		       net_addr_t tSrcAddr, SNetLink ** ppLink);

#ifdef __cplusplus
}
#endif

#endif /* __NET_IPIP_H__ */
