// ==================================================================
// @(#)iface_rtr.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 19/02/2008
// @lastdate05/03/2008
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <net/iface.h>
#include <net/link.h>
#include <net/prefix.h>
#include <net/net_types.h>

// -----[ _net_iface_rtr_send ]--------------------------------------
/**
 * Forward a message along a point-to-point link. The next-hop
 * information is not used.
 */
static int _net_iface_rtr_send(net_addr_t tPhysAddr, void * pContext,
			       SNetIface ** ppDstIface,
			       SNetMessage ** ppMsg)
{
  SNetIface * pIface= (SNetIface *) pContext;

  // Check link's state
  if (!net_iface_is_connected(pIface))
    return NET_ERROR_LINK_DOWN;
  if (!net_iface_is_enabled(pIface))
    return NET_ERROR_LINK_DOWN;

  *ppDstIface= pIface->tDest.pIface;

  return NET_SUCCESS;
}

// -----[ net_iface_new_rtr ]----------------------------------------
SNetIface * net_iface_new_rtr(SNetNode * pNode, net_addr_t tAddr)
{
  SNetIface * pIface= net_iface_new(pNode, NET_IFACE_RTR);
  pIface->tIfaceAddr= tAddr;
  pIface->tIfaceMask= 32;
  pIface->pContext= pIface;
  pIface->fSend= _net_iface_rtr_send;
  return pIface;
}
