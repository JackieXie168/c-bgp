// ==================================================================
// @(#)icmp.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 25/02/2004
// @lastdate 25/02/2004
// ==================================================================

#ifndef __NET_ICMP_H__
#define __NET_ICMP_H__

#include <net/prefix.h>
#include <net/message.h>
#include <net/network.h>

// ----- icmp_echo_request ------------------------------------------
extern int icmp_echo_request(SNetNode * pNode, net_addr_t tDstAddr);
// ----- icmp_handler -----------------------------------------------
extern int icmp_event_handler(void * pHandler, SNetMessage * pMessage);

#endif
