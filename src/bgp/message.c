// ==================================================================
// @(#)message.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be), Sebastien Tandel
// @date 19/05/2003
// @lastdate 25/02/2004
// ==================================================================

#include <assert.h>
#include <string.h>

#include <libgds/memory.h>

#include <bgp/as.h>
#include <bgp/message.h>
#include <net/network.h>
#include <net/protocol.h>
#include <sim/simulator.h>

unsigned long context_create_count= 0;
unsigned long context_destroy_count= 0;

extern SAS * AS[];

typedef struct {
  uint16_t uRemoteAS;
  SBGPMsg * pMsg;
} SBGPMsgSendContext;

static SLog * pMonitor= NULL;

// ----- bgp_msg_update_create --------------------------------------
/**
 *
 */
SBGPMsg * bgp_msg_update_create(uint16_t uPeerAS,
				SRoute * pRoute)
{
  SBGPMsgUpdate * pMsg= (SBGPMsgUpdate *) MALLOC(sizeof(SBGPMsgUpdate));
  pMsg->uType= BGP_MSG_UPDATE;
  pMsg->uPeerAS= uPeerAS;
  pMsg->pRoute= pRoute;
  return (SBGPMsg *) pMsg;
}

// ----- bgp_msg_withdraw_create ------------------------------------
/**
 *
 */
SBGPMsg * bgp_msg_withdraw_create(uint16_t uPeerAS,
				  SPrefix sPrefix)
{
  SBGPMsgWithdraw * pMsg=
    (SBGPMsgWithdraw *) MALLOC(sizeof(SBGPMsgWithdraw));
  pMsg->uType= BGP_MSG_WITHDRAW;
  pMsg->uPeerAS= uPeerAS;
  memcpy(&(pMsg->sPrefix), &sPrefix, sizeof(SPrefix));
  return (SBGPMsg *) pMsg;
}

// ----- bgp_msg_close_create ---------------------------------------
SBGPMsg * bgp_msg_close_create(uint16_t uPeerAS)
{
  SBGPMsgClose * pMsg=
    (SBGPMsgClose *) MALLOC(sizeof(SBGPMsgClose));
  pMsg->uType= BGP_MSG_CLOSE;
  pMsg->uPeerAS= uPeerAS;
  return (SBGPMsg *) pMsg;
}

// ----- bgp_msg_open_create ----------------------------------------
SBGPMsg * bgp_msg_open_create(uint16_t uPeerAS)
{
  SBGPMsgOpen * pMsg=
    (SBGPMsgOpen *) MALLOC(sizeof(SBGPMsgOpen));
  pMsg->uType= BGP_MSG_OPEN;
  pMsg->uPeerAS= uPeerAS;
  return (SBGPMsg *) pMsg;
}

// ----- bgp_msg_destroy --------------------------------------------
/**
 *
 */
void bgp_msg_destroy(SBGPMsg ** ppMsg)
{
  if (*ppMsg != NULL) {
    FREE(*ppMsg);
    *ppMsg= NULL;
  }
}

// ----- bgp_msg_send -----------------------------------------------
/**
 *
 */
void bgp_msg_send(SNetNode * pNode, net_addr_t tAddr, SBGPMsg * pMsg)
{
  bgp_msg_monitor_write(pMsg, pNode, tAddr);
  node_send(pNode, tAddr, NET_PROTOCOL_BGP, pMsg,
	    (FPayLoadDestroy) bgp_msg_destroy);
}

/////////////////////////////////////////////////////////////////////
// BGP MESSAGES MONITORING SECTION
/////////////////////////////////////////////////////////////////////
//
// This is enabled through the CLI with the following command
// (see cli/bgp.c): bgp options msg-monitor <output-file>

// ----- bgp_msg_monitor_open ---------------------------------------
/**
 *
 */
void bgp_msg_monitor_open(char * pcFileName)
{
  if (pMonitor == NULL)
    pMonitor= log_create();
  log_set_file(pMonitor, pcFileName);
  log_set_level(pMonitor, LOG_LEVEL_EVERYTHING);
}

// ----- bgp_msg_monitor_write --------------------------------------
/**
 * Write the given BGP update/withdraw message in MRTD format, i.e.
 * a BGP update message is written as
 *
 *   BGP4|<time>|A|<peer-ip>|<peer-as>|<prefix>|<as-path>|<origin>|
 *   <next-hop>|<local-pref>|<med>|<communities>
 *
 * and a BGP withdraw message is written as
 *
 *   BGP4|<time>|W|<peer-ip>|<peer-as>|<prefix>
 *
 * In addition to this, each message is prefixed with the
 * destination's IP address
 *
 *   <dest-ip>|
 */
void bgp_msg_monitor_write(SBGPMsg * pMsg, SNetNode * pNode,
			   net_addr_t tAddr)
{
  int iIndex;
  uint32_t uCommunity;
  SRoute * pRoute;

  if ((pMonitor != NULL) && (pMonitor->pStream != NULL)) {
    // Destination router (): this is not MRTD format but required to
    // identify the destination of the messages
    ip_address_dump(pMonitor->pStream, tAddr);
    // Protocol and Time
    fprintf(pMonitor->pStream, "|BGP4|%.2f", simulator_get_time());
    // Message type
    switch (pMsg->uType) {
    case BGP_MSG_UPDATE:
      pRoute= ((SBGPMsgUpdate *) pMsg)->pRoute;
      fprintf(pMonitor->pStream, "|A");
      // Peer IP
      fprintf(pMonitor->pStream, "|");
      ip_address_dump(pMonitor->pStream, pNode->tAddr);
      // Peer AS
      fprintf(pMonitor->pStream, "|%d", pMsg->uPeerAS);
      // Prefix
      fprintf(pMonitor->pStream, "|");
      ip_prefix_dump(pMonitor->pStream, pRoute->sPrefix);
      // AS-PATH
      fprintf(pMonitor->pStream, "|");
      if (pMsg->uType == BGP_MSG_UPDATE) {
	path_dump(pMonitor->pStream, pRoute->pASPath, 1);
      }
      // ORIGIN
      fprintf(pMonitor->pStream, "|");
      switch (pRoute->uOriginType) {
      case ROUTE_ORIGIN_IGP:
	fprintf(pMonitor->pStream, "IGP"); break;
      case ROUTE_ORIGIN_EGP:
	fprintf(pMonitor->pStream, "EGP"); break;
      case ROUTE_ORIGIN_INCOMPLETE:
	fprintf(pMonitor->pStream, "INCOMPLETE"); break;
      default:
	fprintf(pMonitor->pStream, "?");
      }
      // NEXT-HOP
      fprintf(pMonitor->pStream, "|");
      ip_address_dump(pMonitor->pStream, pRoute->tNextHop);
      // LOCAL-PREF
      fprintf(pMonitor->pStream, "|%u", pRoute->uLocalPref);
      // MULTI-EXIT-DISCRIMINATOR
      fprintf(pMonitor->pStream, "|0");
      // COMMUNITY
      fprintf(pMonitor->pStream, "|");
      if (pRoute->pCommunities != NULL) {
	for (iIndex= 0; iIndex < pRoute->pCommunities->iSize; iIndex++) {
	  uCommunity= (uint32_t) pRoute->pCommunities->ppItems[iIndex];
	  fprintf(pMonitor->pStream, "%u ", uCommunity);
	}
      }
      break;
    case BGP_MSG_WITHDRAW:
      fprintf(pMonitor->pStream, "|W");
      // Peer IP
      fprintf(pMonitor->pStream, "|");
      ip_address_dump(pMonitor->pStream, pNode->tAddr);
      // Peer AS
      fprintf(pMonitor->pStream, "|%d", pMsg->uPeerAS);
      // Prefix
      fprintf(pMonitor->pStream, "|");
      ip_prefix_dump(pMonitor->pStream, ((SBGPMsgWithdraw *) pMsg)->sPrefix);
      break;
    default:
      fprintf(pMonitor->pStream, "|?");
    }
    fprintf(pMonitor->pStream, "\n");
  }
}

// ----- bgp_msg_monitor_destroy ------------------------------------
/**
 *
 */
void bgp_msg_monitor_destroy(SLog ** ppLog)
{
  log_destroy(ppLog);
}

/////////////////////////////////////////////////////////////////////
// INITIALIZATION AND FINALIZATION SECTION
/////////////////////////////////////////////////////////////////////

void _message_destroy() __attribute__((destructor));

void _message_destroy()
{
  bgp_msg_monitor_destroy(&pMonitor);
}
