// ==================================================================
// @(#)icmp.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 25/02/2004
// @lastdate 28/09/2006
// ==================================================================

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>

#include <net/prefix.h>
#include <net/icmp.h>
#include <net/network.h>

#define ICMP_ECHO_REQUEST 0
#define ICMP_ECHO_REPLY   1

// ----- icmp_echo_request ------------------------------------------
/**
 * Send an ICMP echo request to the given destination address.
 */
int icmp_echo_request(SNetNode * pNode, net_addr_t tDstAddr)
{
  return node_send(pNode, 0, tDstAddr, NET_PROTOCOL_ICMP,
		   (void *) ICMP_ECHO_REQUEST, NULL);
}

// ----- icmp_echo_reply --------------------------------------------
/**
 * Send an ICMP echo reply to the given destination address. The
 * source address of the ICMP message is set to the given source
 * address. Normally, the source address must be the same as the ICMP
 * echo request message's destination address.
 */
int icmp_echo_reply(SNetNode * pNode, net_addr_t tDstAddr,
		    net_addr_t tSrcAddr)
{
  return node_send(pNode, tSrcAddr, tDstAddr, NET_PROTOCOL_ICMP,
		   (void *) ICMP_ECHO_REPLY, NULL);
}

// ----- icmp_handler -----------------------------------------------
/**
 * This function handles ICMP messages receive by a node.
 *
 * This function has signature 'FNetNodeHandleEvent'.
 */
int icmp_event_handler(void * pHandler, SNetMessage * pMessage)
{
  SNetNode * pNode= (SNetNode *) pHandler;
  int iPayLoad= (int) (long) pMessage->pPayLoad;

  switch (iPayLoad) {
  case ICMP_ECHO_REQUEST:
    log_printf(pLogOut, "icmp-echo-request from ");
    ip_address_dump(pLogOut, pMessage->tSrcAddr);
    log_printf(pLogOut, " to ");
    ip_address_dump(pLogOut, pNode->tAddr);
    log_printf(pLogOut, " ttl=%d\n", pMessage->uTTL);
    icmp_echo_reply(pNode, pMessage->tSrcAddr, pMessage->tDstAddr);
    break;
  case ICMP_ECHO_REPLY:
    log_printf(pLogOut, "icmp-echo-reply from ");
    ip_address_dump(pLogOut, pMessage->tSrcAddr);
    log_printf(pLogOut, " ttl=%d\n", pMessage->uTTL);
    break;
  default:
    LOG_ERR(LOG_LEVEL_FATAL, "Error: unsupported ICMP message\n");
    abort();
  }
  return 0;
}
