// ==================================================================
// @(#)icmp.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 25/02/2004
// @lastdate 27/02/2004
// ==================================================================

#include <stdlib.h>

#include <net/prefix.h>
#include <net/icmp.h>
#include <net/network.h>

#define ICMP_ECHO_REQUEST 0
#define ICMP_ECHO_REPLY   1

// ----- icmp_echo_request ------------------------------------------
/**
 *
 */
int icmp_echo_request(SNetNode * pNode, net_addr_t tDstAddr)
{
  return node_send(pNode, tDstAddr, NET_PROTOCOL_ICMP,
		   (void *) ICMP_ECHO_REQUEST, NULL);
}

// ----- icmp_echo_reply --------------------------------------------
/**
 *
 */
int icmp_echo_reply(SNetNode * pNode, net_addr_t tDstAddr)
{
  return node_send(pNode, tDstAddr, NET_PROTOCOL_ICMP,
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
    LOG_INFO("Info: ICMP echo request from \"");
    LOG_ENABLED_INFO() ip_address_dump(log_get_stream(pMainLog),
				       pMessage->tSrcAddr);
    LOG_INFO("\" to \"");
    LOG_ENABLED_INFO() ip_address_dump(log_get_stream(pMainLog),
				       pNode->tAddr);
    LOG_INFO("\"\n");
    icmp_echo_reply(pNode, pMessage->tSrcAddr);
    break;
  case ICMP_ECHO_REPLY:
    LOG_INFO("Info: ICMP echo reply from \"");
    LOG_ENABLED_INFO() ip_address_dump(log_get_stream(pMainLog),
				       pMessage->tSrcAddr);
    LOG_INFO("\" to \"");
    LOG_ENABLED_INFO() ip_address_dump(log_get_stream(pMainLog),
				       pNode->tAddr);
    LOG_INFO("\" with TTL=%u\n", pMessage->uTTL);
    break;
  default:
    LOG_SEVERE("Error: unsupported ICMP message\n");
    abort();
  }
  return 0;
}
