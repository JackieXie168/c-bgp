// ==================================================================
// @(#)ipip.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 25/02/2004
// @lastdate 25/02/2004
// ==================================================================

#ifndef __NET_IPIP_H__
#define __NET_IPIP_H__

#include <net/prefix.h>
#include <net/message.h>
#include <net/network.h>

// ----- ipip_send --------------------------------------------------
extern int ipip_send(SNetNode * pNode, net_addr_t tDstAddr,
		     SNetMessage * pMessage);
// ----- ipip_event_handler -----------------------------------------
extern int ipip_event_handler(void * pHandler, SNetMessage * pMessage);

#endif
