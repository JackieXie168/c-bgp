// ==================================================================
// @(#)iface_ptmp.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 19/02/2008
// @lastdate 12/02/2008
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <assert.h>

#include <net/iface.h>
#include <net/link.h>
#include <net/prefix.h>
#include <net/net_types.h>
#include <net/subnet.h>

// -----[ _net_iface_ptmp_send ]-------------------------------------
static int _net_iface_ptmp_send(net_addr_t tPhysAddr,
				void * pContext,
				SNetIface ** ppDstIface,
				SNetMessage ** ppMsg)
{
  SNetIface * pIface= (SNetIface *) pContext;
  SNetSubnet * pSubnet;
  SNetIface * pDstIface;

  assert(pIface->tType == NET_IFACE_PTMP);
  
  if (!net_iface_is_connected(pIface))
    return NET_ERROR_LINK_DOWN;
  if (!net_iface_is_enabled(pIface))
    return NET_ERROR_LINK_DOWN;

  pSubnet= pIface->tDest.pSubnet;

  // Find destination node (based on "physical address")
  pDstIface= net_subnet_find_link(pSubnet, tPhysAddr);
  if (pDstIface == NULL)
    return NET_ERROR_HOST_UNREACH;

  // Forward along link from subnet -> node ...
  if (!net_iface_is_enabled(pDstIface))
    return NET_ERROR_LINK_DOWN;

  *((SNetIface **) ppDstIface)= pDstIface;
  return NET_SUCCESS;
}

// -----[ net_iface_new_ptmp ]---------------------------------------
SNetIface * net_iface_new_ptmp(SNetNode * pNode, SPrefix sPrefix)
{
  SNetIface * pIface= net_iface_new(pNode, NET_IFACE_PTMP);
  pIface->tIfaceAddr= sPrefix.tNetwork;
  pIface->tIfaceMask= sPrefix.uMaskLen;
  pIface->pContext= pIface;
  pIface->fSend= _net_iface_ptmp_send;
  return pIface;
}



