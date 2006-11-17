// ==================================================================
// @(#)message.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 23/02/2004
// @lastdate 03/03/2006
// ==================================================================

#ifndef __NET_MESSAGE_H__
#define __NET_MESSAGE_H__

#include <libgds/log.h>
#include <net/prefix.h>

typedef void (*FPayLoadDestroy)(void ** pPayLoad);

typedef struct {
  net_addr_t tSrcAddr;
  net_addr_t tDstAddr;
  uint8_t uProtocol;
  uint8_t uTTL;
  void * pPayLoad;
  FPayLoadDestroy fDestroy;
} SNetMessage;

// ----- message_create ---------------------------------------------
extern SNetMessage * message_create(net_addr_t tSrcAddr,
				    net_addr_t tDstAddr,
				    uint8_t uProtocol,
				    uint8_t uTTL,
				    void * pPayLoad,
				    FPayLoadDestroy fDestroy);
// ----- message_destroy --------------------------------------------
extern void message_destroy(SNetMessage ** ppMessage);
// ----- message_dump -----------------------------------------------
extern void message_dump(SLogStream * pStream, SNetMessage * pMessage);

#endif
