// ==================================================================
// @(#)message.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 19/05/2003
// @lastdate 23/02/2004
// ==================================================================

#ifndef __BGP_MESSAGE_H__
#define __BGP_MESSAGE_H__

#include <libgds/types.h>

#include <net/network.h>
#include <bgp/route.h>

#define BGP_MSG_UPDATE   1
#define BGP_MSG_WITHDRAW 2
#define BGP_MSG_CLOSE    3
#define BGP_MSG_OPEN     4

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
} SBGPMsgWithdraw;

typedef struct {
  uint8_t uType;
  uint16_t uPeerAS;
} SBGPMsgClose;

typedef struct {
  uint8_t uType;
  uint16_t uPeerAS;
} SBGPMsgOpen;

// ----- bgp_msg_update_create --------------------------------------
extern SBGPMsg * bgp_msg_update_create(uint16_t uPeerAS,
				       SRoute * pRoute);
// ----- bgp_msg_withdraw_create ------------------------------------
extern SBGPMsg * bgp_msg_withdraw_create(uint16_t uPeerAS,
					 SPrefix sPrefix);
// ----- bgp_msg_close_create ---------------------------------------
extern SBGPMsg * bgp_msg_close_create(uint16_t uPeerAS);
// ----- bgp_msg_open_create ----------------------------------------
extern SBGPMsg * bgp_msg_open_create(uint16_t uPeerAS);

// ----- bgp_msg_destroy --------------------------------------------
extern void bgp_msg_destroy(SBGPMsg ** ppMsg);
// ----- bgp_msg_send -----------------------------------------------
extern void bgp_msg_send(SNetNode * pNode,
			 net_addr_t tAddr, SBGPMsg * pMsg);

// ----- bgp_msg_monitor_open ---------------------------------------
extern void bgp_msg_monitor_open(char * pcFileName);
// ----- bgp_msg_monitor_write --------------------------------------
extern void bgp_msg_monitor_write(SBGPMsg * pMsg, SNetNode * pNode,
				  net_addr_t tAddr);

#endif
