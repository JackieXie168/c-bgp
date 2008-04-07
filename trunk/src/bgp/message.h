// ==================================================================
// @(#)message.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @author Sebastien Tandel (standel@info.ucl.ac.be)
// 
// @date 19/05/2003
// @lastdate 27/02/2008
// ==================================================================

#ifndef __BGP_MESSAGE_H__
#define __BGP_MESSAGE_H__

#include <libgds/types.h>

#include <net/network.h>
#include <bgp/route.h>
#include <bgp/types.h>

typedef enum {
  BGP_MSG_TYPE_UPDATE,
  BGP_MSG_TYPE_WITHDRAW,
  BGP_MSG_TYPE_CLOSE,
  BGP_MSG_TYPE_OPEN,
  BGP_MSG_TYPE_MAX,
} bgp_msg_type_t;

typedef struct {
  bgp_msg_type_t type;
  uint16_t       uPeerAS;
  unsigned int   uSeqNum;
} bgp_msg_t;
typedef bgp_msg_t SBGPMsg;

typedef struct {
  bgp_msg_t     header;
  bgp_route_t * pRoute;
} bgp_msg_update_t;
typedef bgp_msg_update_t SBGPMsgUpdate;

typedef struct {
  bgp_msg_t       header;
  SPrefix         sPrefix;
  net_addr_t    * tNextHop;
} bgp_msg_withdraw_t;
typedef bgp_msg_withdraw_t SBGPMsgWithdraw;

typedef struct {
  bgp_msg_t header;
} bgp_msg_close_t;
typedef bgp_msg_close_t SBGPMsgClose;

typedef struct {
  bgp_msg_t  header;
  net_addr_t router_id;
} bgp_msg_open_t;
typedef bgp_msg_open_t SBGPMsgOpen;

#ifdef _cplusplus
extern "C" {
#endif
  
  // ----- bgp_msg_update_create ------------------------------------
  bgp_msg_t * bgp_msg_update_create(uint16_t uPeerAS,
				    bgp_route_t * pRoute);
  // ----- bgp_msg_withdraw_create ----------------------------------
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  bgp_msg_t * bgp_msg_withdraw_create(uint16_t uPeerAS,
				      SPrefix sPrefix,
				      net_addr_t * tNextHop);
#else
  bgp_msg_t * bgp_msg_withdraw_create(uint16_t uPeerAS,
				      SPrefix sPrefix);
#endif
  // ----- bgp_msg_close_create -------------------------------------
  bgp_msg_t * bgp_msg_close_create(uint16_t uPeerAS);
  // ----- bgp_msg_open_create --------------------------------------
  bgp_msg_t * bgp_msg_open_create(uint16_t uPeerAS,
				  net_addr_t router_id);

  // ----- bgp_msg_destroy ------------------------------------------
  void bgp_msg_destroy(bgp_msg_t ** msg_ref);
  // ----- bgp_msg_send ---------------------------------------------
  int bgp_msg_send(net_node_t * node, net_addr_t src_addr,
		   net_addr_t dst_addr, bgp_msg_t * msg);
  // ----- bgp_msg_dump ---------------------------------------------
  void bgp_msg_dump(SLogStream * pStream, net_node_t * node,
		    bgp_msg_t * msg);

  // ----- bgp_msg_monitor_open -------------------------------------
  int bgp_msg_monitor_open(const char * file_name);
  // -----[ bgp_msg_monitor_close ]----------------------------------
  void bgp_msg_monitor_close();
  // ----- bgp_msg_monitor_write ------------------------------------
  void bgp_msg_monitor_write(bgp_msg_t * msg, net_node_t * node,
			     net_addr_t addr);

  // -----[ _message_destroy ]---------------------------------------
  void _message_destroy();

#ifdef _cplusplus
}
#endif

#endif
