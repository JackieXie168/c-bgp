// ==================================================================
// @(#)message.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @author Sebastien Tandel (standel@info.ucl.ac.be)
// @date 19/05/2003
// @lastdate 10/10/2005
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

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

extern SBGPRouter * AS[];

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
int bgp_msg_send(SNetNode * pNode, net_addr_t tAddr, SBGPMsg * pMsg)
{
  bgp_msg_monitor_write(pMsg, pNode, tAddr);

  //fprintf(stdout, "(");
  //ip_address_dump(stdout, pNode->tAddr);
  //fprintf(stdout, ") bgp-msg-send from ");
  //ip_address_dump(stdout, pNode->tAddr);
  //fprintf(stdout, " to ");
  //ip_address_dump(stdout, tAddr);
  //fprintf(stdout, "\n");
  
  return node_send(pNode, 0, tAddr, NET_PROTOCOL_BGP, pMsg,
		   (FPayLoadDestroy) bgp_msg_destroy);
}

// ----- bgp_msg_dump -----------------------------------------------
/**
 *
 */
void bgp_msg_dump(FILE * pStream, SNetNode * pNode, SBGPMsg * pMsg)
{
  int iIndex;
  uint32_t uCommunity;
  SRoute * pRoute;

  // Message type
  switch (pMsg->uType) {
  case BGP_MSG_UPDATE:
    pRoute= ((SBGPMsgUpdate *) pMsg)->pRoute;
    fprintf(pStream, "|A");
    // Peer IP
    fprintf(pStream, "|");
    if (pNode != NULL) {
      ip_address_dump(pStream, pNode->tAddr);
    } else {
      fprintf(pStream, "?");
    }
    // Peer AS
    fprintf(pStream, "|%d", pMsg->uPeerAS);
    // Prefix
    fprintf(pStream, "|");
    ip_prefix_dump(pStream, pRoute->sPrefix);
    // AS-PATH
    fprintf(pStream, "|");
    if (pMsg->uType == BGP_MSG_UPDATE) {
      path_dump(pStream, route_path_get(pRoute), 1);
    }
    // ORIGIN
    fprintf(pStream, "|");
    switch (pRoute->uOriginType) {
    case ROUTE_ORIGIN_IGP:
      fprintf(pStream, "IGP"); break;
    case ROUTE_ORIGIN_EGP:
      fprintf(pStream, "EGP"); break;
    case ROUTE_ORIGIN_INCOMPLETE:
      fprintf(pStream, "INCOMPLETE"); break;
    default:
      fprintf(pStream, "?");
    }
    // NEXT-HOP
    fprintf(pStream, "|");
    ip_address_dump(pStream, pRoute->tNextHop);
    // LOCAL-PREF
    fprintf(pStream, "|%u", pRoute->uLocalPref);
    // MULTI-EXIT-DISCRIMINATOR
    if (pRoute->uMED == ROUTE_MED_MISSING)
      fprintf(pStream, "|");
    else
      fprintf(pStream, "|%u", pRoute->uMED);
    // COMMUNITY
    fprintf(pStream, "|");
    if (pRoute->pCommunities != NULL) {
      for (iIndex= 0; iIndex < pRoute->pCommunities->iSize; iIndex++) {
	uCommunity= (uint32_t) pRoute->pCommunities->ppItems[iIndex];
	fprintf(pStream, "%u ", uCommunity);
      }
    }

    // Route-reflectors: Originator
    if (pRoute->pOriginator != NULL) {
      fprintf(pStream, "originator:");
      ip_address_dump(pStream, *pRoute->pOriginator);
    }
    fprintf(pStream, "|");

    if (pRoute->pClusterList != NULL) {
      fprintf(pStream, "cluster_id_list:");
      cluster_list_dump(pStream, pRoute->pClusterList);
    }
    fprintf(pStream, "|");

    break;
  case BGP_MSG_WITHDRAW:
    fprintf(pStream, "|W");
    // Peer IP
    fprintf(pStream, "|");
    if (pNode != NULL) {
      ip_address_dump(pStream, pNode->tAddr);
    } else {
      fprintf(pStream, "?");
    }
    // Peer AS
    fprintf(pStream, "|%d", pMsg->uPeerAS);
    // Prefix
    fprintf(pStream, "|");
    ip_prefix_dump(pStream, ((SBGPMsgWithdraw *) pMsg)->sPrefix);
    break;
  case BGP_MSG_OPEN:
    fprintf(pStream, "|OPEN|");
    if (pNode != NULL) {
      ip_address_dump(pStream, pNode->tAddr);
    } else {
      fprintf(pStream, "?");
    }
    break;
  case BGP_MSG_CLOSE:
    fprintf(pStream, "|CLOSE|");
    if (pNode != NULL) {
      ip_address_dump(pStream, pNode->tAddr);
    } else {
      fprintf(pStream, "?");
    }
    break;
  default:
    fprintf(pStream, "|?");
  } 
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
  if ((pMonitor != NULL) && (pMonitor->pStream != NULL)) {

    // Destination router (): this is not MRTD format but required to
    // identify the destination of the messages
    ip_address_dump(pMonitor->pStream, tAddr);
    // Protocol and Time
    fprintf(pMonitor->pStream, "|BGP4|%.2f", simulator_get_time());

    bgp_msg_dump(pMonitor->pStream, pNode, pMsg);

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

// -----[ -message_destroy ]-----------------------------------------
void _message_destroy()
{
  bgp_msg_monitor_destroy(&pMonitor);
}
