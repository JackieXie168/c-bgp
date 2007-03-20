// ==================================================================
// @(#)message.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 23/02/2004
// @lastdate 19/01/2007
// ==================================================================

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <libgds/log.h>
#include <libgds/memory.h>

#include <net/message.h>
#include <net/protocol.h>

// ----- message_create ---------------------------------------------
/**
 *
 */
SNetMessage * message_create(net_addr_t tSrcAddr, net_addr_t tDstAddr,
			     uint8_t uProtocol, uint8_t uTTL,
			     void * pPayLoad, FPayLoadDestroy fDestroy)
{
  SNetMessage * pMessage=
    (SNetMessage *) MALLOC(sizeof(SNetMessage));
  pMessage->tSrcAddr= tSrcAddr;
  pMessage->tDstAddr= tDstAddr;
  pMessage->uProtocol= uProtocol;
  pMessage->uTTL= uTTL;
  pMessage->tTOS= 0;
  pMessage->pPayLoad= pPayLoad;
  pMessage->fDestroy= fDestroy;
  return pMessage;
}

// ----- message_destroy --------------------------------------------
/**
 *
 */
void message_destroy(SNetMessage ** ppMessage)
{
  if (*ppMessage != NULL) {
    if ((*ppMessage)->fDestroy != NULL)
      (*ppMessage)->fDestroy(&(*ppMessage)->pPayLoad);
    FREE(*ppMessage);
  }
}

// ----- message_dump -----------------------------------------------
/**
 *
 */
void message_dump(SLogStream * pStream, SNetMessage * pMessage)
{
  log_printf(pStream, "src:");
  ip_address_dump(pStream, pMessage->tSrcAddr);
  log_printf(pStream, ", dst:");
  ip_address_dump(pStream, pMessage->tDstAddr);
  log_printf(pStream, ", proto:%s, ttl:%d",
	     PROTOCOL_NAMES[pMessage->uProtocol], pMessage->uTTL);
}
