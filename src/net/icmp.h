// ==================================================================
// @(#)icmp.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 25/02/2004
// @lastdate 18/04/2007
// ==================================================================

#ifndef __NET_ICMP_H__
#define __NET_ICMP_H__

#include <net/prefix.h>
#include <net/message.h>
#include <net/network.h>

#define ICMP_ERROR_NET_UNREACH   1
#define ICMP_ERROR_DST_UNREACH   2
#define ICMP_ERROR_PORT_UNREACH  3
#define ICMP_ERROR_TIME_EXCEEDED 4

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ icmp_send_error ]----------------------------------------
  int icmp_send_error(SNetNode * pNode, net_addr_t tSrcAddr,
		      net_addr_t tDstAddr, uint8_t uErrorCode,
		      SSimulator * pSimulator);
  // -----[ icmp_send_echo_request ]---------------------------------
  int icmp_send_echo_request(SNetNode * pNode, net_addr_t tSrcAddr,
			net_addr_t tDstAddr, uint8_t uTTL,
			SSimulator * pSimulator);
  // -----[ icmp_event_handler ]-------------------------------------
  int icmp_event_handler(SSimulator * pSimulator,
			 void * pHandler, SNetMessage * pMessage);
  // -----[ is_icmp_error ]------------------------------------------
  int is_icmp_error(SNetMessage * pMessage);
  // -----[ icmp_ping_send_recv ]------------------------------------
  int icmp_ping_send_recv(SNetNode * pSrcNode, net_addr_t tSrcAddr,
			  net_addr_t tDstAddr, uint8_t uTTL);
  
  ///////////////////////////////////////////////////////////////////
  // PROBES
  ///////////////////////////////////////////////////////////////////

  // -----[ icmp_ping ]----------------------------------------------
  int icmp_ping(SLogStream * pStream,
		SNetNode * pSrcNode, net_addr_t tSrcAddr,
		net_addr_t tDstAddr, uint8_t uTTL);
  // -----[ icmp_trace_route ]---------------------------------------
  int icmp_trace_route(SLogStream * pStream,
		       SNetNode * pSrcNode, net_addr_t tSrcAddr,
		       net_addr_t tDstAddr, uint8_t uMaxTTL);
  
#ifdef __cplusplus
}
#endif

#endif
