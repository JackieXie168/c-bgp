// ==================================================================
// @(#)icmp.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 25/02/2004
// @lastdate 08/08/2005
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

  switch ((int) pMessage->pPayLoad) {
  case ICMP_ECHO_REQUEST:
    fprintf(stdout, "icmp-echo-request from ");
    ip_address_dump(stdout, pMessage->tSrcAddr);
    fprintf(stdout, " to ");
    ip_address_dump(stdout, pNode->tAddr);
    fprintf(stdout, " ttl=%d\n", pMessage->uTTL);
    icmp_echo_reply(pNode, pMessage->tSrcAddr, pMessage->tDstAddr);
    break;
  case ICMP_ECHO_REPLY:
    fprintf(stdout, "icmp-echo-reply from ");
    ip_address_dump(stdout, pMessage->tSrcAddr);
    fprintf(stdout, " ttl=%d\n", pMessage->uTTL);
    break;
  default:
    LOG_SEVERE("Error: unsupported ICMP message\n");
    abort();
  }
  return 0;
}
