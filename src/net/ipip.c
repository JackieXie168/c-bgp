// ==================================================================
// @(#)ipip.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 25/02/2004
// @lastdate 25/02/2004
// ==================================================================

#include <net/ipip.h>
#include <net/protocol.h>

// ----- ipip_send --------------------------------------------------
/**
 * Encapsulate the given message and sent it to the given tunnel
 * endpoint address.
 */
int ipip_send(SNetNode * pNode, net_addr_t tDstAddr,
	      SNetMessage * pMessage)
{
  return 0;
}

// ----- ipip_event_handler -----------------------------------------
/**
 * IP-in-IP protocol handler. Decapsulate IP-in-IP messages and send
 * to encapsulated message's destination.
 */
int ipip_event_handler(void * pHandler, SNetMessage * pMessage)
{
  //SNetNode * pNode= (SNetNode *) pHandler;

  return 0;
}
