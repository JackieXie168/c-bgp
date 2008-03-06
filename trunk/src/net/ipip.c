// ==================================================================
// @(#)ipip.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 25/02/2004
// @lastdate 15/02/2008
// ==================================================================

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>

#include <net/error.h>
#include <net/iface.h>
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

// -----[ _ipip_link_destroy ]---------------------------------------
static void _ipip_link_destroy(void * pContext)
{
  FREE(pContext);
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

// -----[ ipip_iface_send ]------------------------------------------
/**
 *
 */
static int _ipip_iface_send(net_addr_t tNextHop,
			    void * pContext,
			    SNetIface ** ppIface,
			    SNetMessage ** ppMsg)
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
  return net_iface_send(pTunnel->pOutIface, ppIface,
			pTunnel->tGateway, ppMsg); 
}

// -----[ ipip_iface_recv ]------------------------------------------
/**
 * TODO: destroy payload.
 */
static int _ipip_iface_recv(SNetIface * pIface, SNetMessage * pMsg)
{
  return node_recv_msg(pIface->pSrcNode, pIface,
		       (SNetMessage *) pMsg->pPayLoad);
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
  SNetIface * pIface;
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
  net_iface_factory(pNode, net_prefix(tAddr, 32), NET_IFACE_VIRTUAL, &pIface);

  pIface->pWeights= net_igp_weights_create(1);
  pIface->tDelay= 0;
  pIface->tCapacity= 0;
  pIface->tLoad= 0;
  pIface->uFlags= NET_LINK_FLAG_UP;

  pIface->tDest.tEndPoint= tDstPoint;

  // Create the IPIP context
  pContext= (SIPIPContext *) MALLOC(sizeof(SIPIPContext));
  pContext->pIface= pIface;
  pContext->pOutIface= pOutIface;
  pContext->tGateway= tGateway;
  pContext->tSrcAddr= tSrcAddr;

  pIface->pContext= pContext;
  pIface->fSend= _ipip_iface_send;
  pIface->fRecv= _ipip_iface_recv;
  pIface->fDestroy= _ipip_link_destroy;

  *ppLink= pIface;

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
/*int ipip_event_handler(SSimulator * pSimulator,
		       void * pHandler, SNetMessage * pMessage)
{
		       }*/
