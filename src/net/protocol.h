// ==================================================================
// @(#)protocol.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 25/02/2004
// @lastdate 09/03/2004
// ==================================================================
// Note: the number of supported protocols will often be low, so that
// the list of protocols is implemented by a fixed-size array at this
// time. Later, if the number of protocols increases or if the
// protocols supported on different nodes varies a lot, we will switch
// to another data structure such as a sorted heap with a O(log(n))
// complexity.


#ifndef __NET_PROTOCOL_H__
#define __NET_PROTOCOL_H__

#include <libgds/array.h>
#include <net/message.h>

// Maximum number of supported protocols
#define NET_PROTOCOL_MAX  3

// Protocol numbers
#define NET_PROTOCOL_ICMP 0
#define NET_PROTOCOL_BGP  1
#define NET_PROTOCOL_IPIP 2

// ----- FNetNodeHandleEvent -----
typedef int (*FNetNodeHandleEvent)(void * pHandler,
				   SNetMessage * pMessage);
// ----- FNetNodeHandlerDestroy -----
typedef void (*FNetNodeHandlerDestroy)(void ** ppHandler);

// ----- SNetProtocol -----
typedef struct {
  void * pHandler;
  FNetNodeHandleEvent fHandleEvent;
  FNetNodeHandlerDestroy fDestroy;
} SNetProtocol;

// ----- SNetProtocols -----
typedef struct {
  SNetProtocol * data[NET_PROTOCOL_MAX];
} SNetProtocols;

// ----- protocols_create -------------------------------------------
extern SNetProtocols * protocols_create();
// ----- protocols_destroy ------------------------------------------
extern void protocols_destroy(SNetProtocols ** ppProtocols);
// ----- protocols_register -----------------------------------------
extern int protocols_register(SNetProtocols * pProtocols,
			      uint8_t uNumber, void * pHandler,
			      FNetNodeHandlerDestroy fDestroy,
			      FNetNodeHandleEvent fHandleEvent);
// ----- protocols_get ----------------------------------------------
extern SNetProtocol * protocols_get(SNetProtocols * pProtocols,
				    uint8_t uNumber);

#endif
