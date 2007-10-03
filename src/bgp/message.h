// ==================================================================
// @(#)message.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @author Sebastien Tandel (standel@info.ucl.ac.be)
// 
// @date 19/05/2003
// @lastdate 18/09/2007
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

typedef struct {
  uint8_t uType;
  uint16_t uPeerAS;
  unsigned int uSeqNum;
} SBGPMsg;

typedef struct {
  uint8_t uType;
  uint16_t uPeerAS;
  unsigned int uSeqNum;
  SRoute * pRoute;
} SBGPMsgUpdate;

typedef struct {
  uint8_t uType;
  uint16_t uPeerAS;
  unsigned int uSeqNum;
  SPrefix sPrefix;
  net_addr_t * tNextHop;
#ifdef __EXPERIMENTAL__
  SBGPRootCause * pRootCause;
#endif
} SBGPMsgWithdraw;

typedef struct {
  uint8_t uType;
  uint16_t uPeerAS;
  unsigned int uSeqNum;
} SBGPMsgClose;

typedef struct {
  uint8_t uType;
  uint16_t uPeerAS;
  unsigned int uSeqNum;
  net_addr_t tRouterID;
} SBGPMsgOpen;

#ifdef _cplusplus
extern "C" {
#endif
  
  // ----- bgp_msg_update_create ------------------------------------
  SBGPMsg * bgp_msg_update_create(uint16_t uPeerAS,
				       SRoute * pRoute);
  // ----- bgp_msg_withdraw_create ----------------------------------
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  SBGPMsg * bgp_msg_withdraw_create(uint16_t uPeerAS,
				    SPrefix sPrefix,
				    net_addr_t * tNextHop);
#else
  SBGPMsg * bgp_msg_withdraw_create(uint16_t uPeerAS,
				    SPrefix sPrefix);
#endif
#ifdef __EXPERIMENTAL__
  // ----- bgp_msg_withdraw_create_rcn ------------------------------
  SBGPMsg * bgp_msg_withdraw_create_rcn(uint16_t uPeerAS,
					SPrefix sPrefix,
					SBGPRootCause * pRootCause);
#endif
  // ----- bgp_msg_close_create -------------------------------------
  SBGPMsg * bgp_msg_close_create(uint16_t uPeerAS);
  // ----- bgp_msg_open_create --------------------------------------
  SBGPMsg * bgp_msg_open_create(uint16_t uPeerAS,
				net_addr_t tRouterID);

  // ----- bgp_msg_destroy ------------------------------------------
  void bgp_msg_destroy(SBGPMsg ** ppMsg);
  // ----- bgp_msg_send ---------------------------------------------
  int bgp_msg_send(SNetNode * pNode,
		   net_addr_t tAddr, SBGPMsg * pMsg);
  // ----- bgp_msg_dump ---------------------------------------------
  void bgp_msg_dump(SLogStream * pStream, SNetNode * pNode, SBGPMsg * pMsg);

  // ----- bgp_msg_monitor_open -------------------------------------
  int bgp_msg_monitor_open(char * pcFileName);
  // -----[ bgp_msg_monitor_close ]----------------------------------
  void bgp_msg_monitor_close();
  // ----- bgp_msg_monitor_write ------------------------------------
  void bgp_msg_monitor_write(SBGPMsg * pMsg, SNetNode * pNode,
			     net_addr_t tAddr);

  // -----[ _message_destroy ]---------------------------------------
  void _message_destroy();

#ifdef _cplusplus
}
#endif

#endif
