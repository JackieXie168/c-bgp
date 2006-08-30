// ==================================================================
// @(#)message.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @author Sebastien Tandel (standel@info.ucl.ac.be)
// @date 19/05/2003
// @lastdate 11/04/2006
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <assert.h>
#include <string.h>
#include <time.h>

#include <libgds/log.h>
#include <libgds/memory.h>
#include <libgds/log.h>

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

static SLogStream * pMonitor= NULL;

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
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
SBGPMsg * bgp_msg_withdraw_create(uint16_t uPeerAS,
				  SPrefix sPrefix, net_addr_t * tNextHop)
#else
SBGPMsg * bgp_msg_withdraw_create(uint16_t uPeerAS,
				  SPrefix sPrefix)
#endif
{
  SBGPMsgWithdraw * pMsg=
    (SBGPMsgWithdraw *) MALLOC(sizeof(SBGPMsgWithdraw));
  pMsg->uType= BGP_MSG_WITHDRAW;
  pMsg->uPeerAS= uPeerAS;
  memcpy(&(pMsg->sPrefix), &sPrefix, sizeof(SPrefix));
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  //It corresponds to the path identifier described in the walton draft 
  //... nevertheless, we keep the variable name NextHop!
//  LOG_DEBUG("creation of withdraw : ");
//  LOG_ENABLED_DEBUG() ip_address_dump(log_get_stream(pMainLog), tNextHop);
//  LOG_DEBUG("\n");
  if (tNextHop != NULL) {
    pMsg->tNextHop = MALLOC(sizeof(net_addr_t));
    memcpy(pMsg->tNextHop, tNextHop, sizeof(net_addr_t));
  } else {
    pMsg->tNextHop = NULL;
  }
#endif
  return (SBGPMsg *) pMsg;
}

#ifdef __EXPERIMENTAL__
// ----- bgp_msg_withdraw_create_rcn --------------------------------
/**
 * Create a BGP withdraw message with root cause notification (RCN).
 */
SBGPMsg * bgp_msg_withdraw_create_rcn(uint16_t uPeerAS,
				      SPrefix sPrefix,
                                      SBGPRootCause * pRootCause)
{
  /*SBGPMsgWithdraw * pMsg= (SBGPMsgWithdraw *)
    bgp_msg_withdraw_create(uPeerAS, sPrefix);
  pMsg->pRootCause= pRootCause;
  return (SBGPMsg *) pMsg;*/
  return NULL;
}
#endif

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
SBGPMsg * bgp_msg_open_create(uint16_t uPeerAS, net_addr_t tRouterID)
{
  SBGPMsgOpen * pMsg=
    (SBGPMsgOpen *) MALLOC(sizeof(SBGPMsgOpen));
  pMsg->uType= BGP_MSG_OPEN;
  pMsg->uPeerAS= uPeerAS;
  pMsg->tRouterID= tRouterID;
  return (SBGPMsg *) pMsg;
}

// ----- bgp_msg_destroy --------------------------------------------
/**
 *
 */
void bgp_msg_destroy(SBGPMsg ** ppMsg)
{
  if (*ppMsg != NULL) {
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
    if ((*ppMsg)->uType == BGP_MSG_WITHDRAW && ((SBGPMsgWithdraw *)(*ppMsg))->tNextHop != NULL)
      FREE( ((SBGPMsgWithdraw *)(*ppMsg))->tNextHop );
#endif
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
void bgp_msg_dump(SLogStream * pStream, SNetNode * pNode, SBGPMsg * pMsg)
{
  int iIndex;
  uint32_t uCommunity;
  SRoute * pRoute;

  // Message type
  switch (pMsg->uType) {
  case BGP_MSG_UPDATE:
    pRoute= ((SBGPMsgUpdate *) pMsg)->pRoute;
    log_printf(pStream, "|A");
    // Peer IP
    log_printf(pStream, "|");
    if (pNode != NULL) {
      ip_address_dump(pStream, pNode->tAddr);
    } else {
      log_printf(pStream, "?");
    }
    // Peer AS
    log_printf(pStream, "|%d", pMsg->uPeerAS);
    // Prefix
    log_printf(pStream, "|");
    ip_prefix_dump(pStream, pRoute->sPrefix);
    // AS-PATH
    log_printf(pStream, "|");
    if (pMsg->uType == BGP_MSG_UPDATE) {
      path_dump(pStream, route_get_path(pRoute), 1);
    }
    // ORIGIN
    log_printf(pStream, "|");
    switch (pRoute->pAttr->tOrigin) {
    case ROUTE_ORIGIN_IGP:
      log_printf(pStream, "IGP"); break;
    case ROUTE_ORIGIN_EGP:
      log_printf(pStream, "EGP"); break;
    case ROUTE_ORIGIN_INCOMPLETE:
      log_printf(pStream, "INCOMPLETE"); break;
    default:
      log_printf(pStream, "?");
    }
    // NEXT-HOP
    log_printf(pStream, "|");
    ip_address_dump(pStream, pRoute->pAttr->tNextHop);
    // LOCAL-PREF
    log_printf(pStream, "|%u", pRoute->pAttr->uLocalPref);
    // MULTI-EXIT-DISCRIMINATOR
    if (pRoute->pAttr->uMED == ROUTE_MED_MISSING)
      log_printf(pStream, "|");
    else
      log_printf(pStream, "|%u", pRoute->pAttr->uMED);
    // COMMUNITY
    log_printf(pStream, "|");
    if (pRoute->pAttr->pCommunities != NULL) {
      for (iIndex= 0; iIndex < pRoute->pAttr->pCommunities->iSize; iIndex++) {
	uCommunity= (uint32_t) pRoute->pAttr->pCommunities->ppItems[iIndex];
	log_printf(pStream, "%u ", uCommunity);
      }
    }

    // Route-reflectors: Originator
    if (pRoute->pOriginator != NULL) {
      log_printf(pStream, "originator:");
      ip_address_dump(pStream, *pRoute->pOriginator);
    }
    log_printf(pStream, "|");

    if (pRoute->pClusterList != NULL) {
      log_printf(pStream, "cluster_id_list:");
      cluster_list_dump(pStream, pRoute->pClusterList);
    }
    log_printf(pStream, "|");

    break;
  case BGP_MSG_WITHDRAW:
    log_printf(pStream, "|W");
    // Peer IP
    log_printf(pStream, "|");
    if (pNode != NULL) {
      ip_address_dump(pStream, pNode->tAddr);
    } else {
      log_printf(pStream, "?");
    }
    // Peer AS
    log_printf(pStream, "|%d", pMsg->uPeerAS);
    // Prefix
    log_printf(pStream, "|");
    ip_prefix_dump(pStream, ((SBGPMsgWithdraw *) pMsg)->sPrefix);
    break;
  case BGP_MSG_OPEN:
    log_printf(pStream, "|OPEN|");
    if (pNode != NULL) {
      ip_address_dump(pStream, pNode->tAddr);
    } else {
      log_printf(pStream, "?");
    }
    break;
  case BGP_MSG_CLOSE:
    log_printf(pStream, "|CLOSE|");
    if (pNode != NULL) {
      ip_address_dump(pStream, pNode->tAddr);
    } else {
      log_printf(pStream, "?");
    }
    break;
  default:
    log_printf(pStream, "|?");
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
int bgp_msg_monitor_open(char * pcFileName)
{
  time_t tTime= time(NULL);

  if (pMonitor == NULL) {
    pMonitor= log_create_file(pcFileName);
    if (pMonitor == NULL) {
      LOG_ERR(LOG_LEVEL_SEVERE, "Unable to create monitor file\n");
      return -1;
    }
  }
  log_set_level(pMonitor, LOG_LEVEL_EVERYTHING);
  log_printf(pMonitor, "# BGP message trace\n");
  log_printf(pMonitor, "# generated by C-BGP on %s",
	  ctime(&tTime));
  log_printf(pMonitor, "# <dest-ip>|BGP4|<event-time>|<type>|"
	  "<peer-ip>|<peer-as>|<prefix>|...\n");
  return 0;
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
  if (pMonitor != NULL) {

    // Destination router (): this is not MRTD format but required to
    // identify the destination of the messages
    ip_address_dump(pMonitor, tAddr);
    // Protocol and Time
    log_printf(pMonitor, "|BGP4|%.2f", simulator_get_time());

    bgp_msg_dump(pMonitor, pNode, pMsg);

    log_printf(pMonitor, "\n");
  }
}

// ----- bgp_msg_monitor_destroy ------------------------------------
/**
 *
 */
void bgp_msg_monitor_destroy(SLogStream ** ppStream)
{
  log_destroy(ppStream);
}

/////////////////////////////////////////////////////////////////////
// INITIALIZATION AND FINALIZATION SECTION
/////////////////////////////////////////////////////////////////////

// -----[ -message_destroy ]-----------------------------------------
void _message_destroy()
{
  bgp_msg_monitor_destroy(&pMonitor);
}
