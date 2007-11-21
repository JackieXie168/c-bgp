// ==================================================================
// @(#)ipip.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 25/02/2004
// @lastdate 23/07/2007
// ==================================================================

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>

#include <net/error.h>
#include <net/ipip.h>
#include <net/link.h>
#include <net/network.h>
#include <net/protocol.h>

typedef struct {
  SNetLink   * pIface;
  SNetLink   * pOutIface;
  net_addr_t   tGateway;
  net_addr_t   tSrcAddr;
} SIPIPContext;

// -----[ _ipip_link_forward ]---------------------------------------
static int _ipip_link_forward(net_addr_t tNextHop, void * pContext,
			      SNetNode ** ppNextHop, SNetMessage ** ppMsg);

// -----[ _ipip_link_destroy ]---------------------------------------
static void _ipip_link_destroy(void * pContext)
{
  FREE(pContext);
}

// -----[ ipip_link_create ]-----------------------------------------
/**
 * Create a tunnel interface (link).
 *
 * Arguments:
 *   node     : source node (where the interface is attached.
 *   dst-point: remote tunnel endpoint
 *   addr     : tunnel identifier
 */
int ipip_link_create(SNetNode * pNode, net_addr_t tDstPoint,
		     net_addr_t tAddr, SNetLink * pOutIface,
		     net_addr_t tSrcAddr,
		     SNetLink ** ppLink)
{
  SNetLink * pLink;
  SIPIPContext * pContext;
  SNetRouteNextHop * pRouteNH;
  net_addr_t tGateway= NET_ADDR_ANY;

  // Retrieve the outgoing interface used to reach the tunnel endpoint
  if (pOutIface == NULL) {
    pRouteNH= node_rt_lookup(pNode, tDstPoint);
    if (pRouteNH == NULL)
      return NET_ERROR_NET_UNREACH;
    pOutIface= pRouteNH->pIface;
    tGateway= pRouteNH->tGateway;
  }

  // Create the link (could be replaced by the link initialization
  // function in link.c)
  pLink=  (SNetLink *) MALLOC(sizeof(SNetLink));
  pLink->pSrcNode= pNode;
  pLink->pWeights= net_igp_weights_create(1);
  pLink->tDelay= 0;
  pLink->tCapacity= 0;
  pLink->tLoad= 0;
  pLink->uFlags= NET_LINK_FLAG_UP;

  pLink->tIfaceAddr= tAddr;
  pLink->tIfaceMask= 32;
  pLink->uType= NET_LINK_TYPE_TUNNEL;
  
  pLink->tDest.tEndPoint= tDstPoint;

  // Create the IPIP context
  pContext= (SIPIPContext *) MALLOC(sizeof(SIPIPContext));
  pContext->pIface= pLink;
  pContext->pOutIface= pOutIface;
  pContext->tGateway= tGateway;
  pContext->tSrcAddr= tSrcAddr;

  pLink->pContext= pContext;
  pLink->fForward= _ipip_link_forward;
  pLink->fHandle= NULL;
  pLink->fDestroy= _ipip_link_destroy;

  *ppLink= pLink;

  return NET_SUCCESS;
}

// ----- ipip_send --------------------------------------------------
/**
 * Encapsulate the given message and sent it to the given tunnel
 * endpoint address.
 */
int ipip_send(SNetNode * pNode, net_addr_t tDstAddr,
	      SNetMessage * pMessage)
{
  return 0;
}

// -----[ ipip_event_handler ]---------------------------------------
/**
 * IP-in-IP protocol handler. Decapsulate IP-in-IP messages and send
 * to encapsulated message's destination.
 */
int ipip_event_handler(SSimulator * pSimulator,
		       void * pHandler, SNetMessage * pMessage)
{
  SNetNode * pNode= (SNetNode *) pHandler;

  // TODO: destroy payload.

  return node_recv_msg(pNode, (SNetMessage *) pMessage->pPayLoad,
		       pSimulator);
}

// -----[ _ipip_msg_destroy ]----------------------------------------
/**
 *
 */
static void _ipip_msg_destroy(void ** ppPayLoad)
{
  /*
  SNetMessage * pMessage= *((SNetMessage **) ppPayLoad);
  message_destroy(&pMessage);
  */
}

// -----[ ipip_link_forward ]----------------------------------------
/**
 *
 */
static int _ipip_link_forward(net_addr_t tNextHop, void * pContext,
			      SNetNode ** ppNextHop, SNetMessage ** ppMsg)
{
  SIPIPContext * pTunnel= (SIPIPContext *) pContext;
  SNetLink * pTunnelIface= pTunnel->pIface;
  net_addr_t tSrcAddr= pTunnel->tSrcAddr;

  // Default IP encap source address = outgoing interface's address.
  if (tSrcAddr == NET_ADDR_ANY)
    tSrcAddr= pTunnel->pOutIface->tIfaceAddr;

  // Build encapsulation message
  *ppMsg= message_create(tSrcAddr,
			 pTunnelIface->tDest.tEndPoint,
			 NET_PROTOCOL_IPIP,
			 255, *ppMsg, _ipip_msg_destroy);

  // Send message through next interface
  return pTunnel->pOutIface->fForward(pTunnel->tGateway,
				      pTunnel->pOutIface->pContext,
				      ppNextHop, ppMsg);
}
