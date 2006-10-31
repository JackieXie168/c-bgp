// ==================================================================
// @(#)message.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @author Sebastien Tandel (standel@info.ucl.ac.be)
// 
// @date 19/05/2003
// @lastdate 11/04/2006
// ==================================================================

#ifndef __BGP_MESSAGE_H__
#define __BGP_MESSAGE_H__

#include <libgds/types.h>

#include <net/network.h>
#include <bgp/route.h>
#include <bgp/types.h>

#define BGP_MSG_UPDATE   0x01
#define BGP_MSG_WITHDRAW 0x02
#define BGP_MSG_CLOSE    0x03
#define BGP_MSG_OPEN     0x04

extern unsigned long context_create_count;
extern unsigned long context_destroy_count;

typedef struct {
  uint8_t uType;
  uint16_t uPeerAS;
} SBGPMsg;

typedef struct {
  uint8_t uType;
  uint16_t uPeerAS;
  SRoute * pRoute;
} SBGPMsgUpdate;

typedef struct {
  uint8_t uType;
  uint16_t uPeerAS;
  SPrefix sPrefix;
  net_addr_t * tNextHop;
#ifdef __EXPERIMENTAL__
  SBGPRootCause * pRootCause;
#endif
} SBGPMsgWithdraw;

typedef struct {
  uint8_t uType;
  uint16_t uPeerAS;
} SBGPMsgClose;

typedef struct {
  uint8_t uType;
  uint16_t uPeerAS;
  net_addr_t tRouterID;
} SBGPMsgOpen;

// ----- bgp_msg_update_create --------------------------------------
extern SBGPMsg * bgp_msg_update_create(uint16_t uPeerAS,
				       SRoute * pRoute);
// ----- bgp_msg_withdraw_create ------------------------------------
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
extern SBGPMsg * bgp_msg_withdraw_create(uint16_t uPeerAS,
					 SPrefix sPrefix,
					 net_addr_t * tNextHop);
#else
extern SBGPMsg * bgp_msg_withdraw_create(uint16_t uPeerAS,
					 SPrefix sPrefix);
#endif
#ifdef __EXPERIMENTAL__
// ----- bgp_msg_withdraw_create_rcn --------------------------------
extern SBGPMsg * bgp_msg_withdraw_create_rcn(uint16_t uPeerAS,
					     SPrefix sPrefix,
                                             SBGPRootCause * pRootCause);
#endif
// ----- bgp_msg_close_create ---------------------------------------
extern SBGPMsg * bgp_msg_close_create(uint16_t uPeerAS);
// ----- bgp_msg_open_create ----------------------------------------
extern SBGPMsg * bgp_msg_open_create(uint16_t uPeerAS,
				     net_addr_t tRouterID);

// ----- bgp_msg_destroy --------------------------------------------
extern void bgp_msg_destroy(SBGPMsg ** ppMsg);
// ----- bgp_msg_send -----------------------------------------------
extern int bgp_msg_send(SNetNode * pNode,
			net_addr_t tAddr, SBGPMsg * pMsg);
// ----- bgp_msg_dump -----------------------------------------------
extern void bgp_msg_dump(SLogStream * pStream, SNetNode * pNode, SBGPMsg * pMsg);

// ----- bgp_msg_monitor_open ---------------------------------------
extern int bgp_msg_monitor_open(char * pcFileName);
// ----- bgp_msg_monitor_write --------------------------------------
extern void bgp_msg_monitor_write(SBGPMsg * pMsg, SNetNode * pNode,
				  net_addr_t tAddr);

// -----[ _message_destroy ]-----------------------------------------
extern void _message_destroy();

#endif
