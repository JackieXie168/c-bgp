// ==================================================================
// @(#)iface_ptp.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 19/02/2008
// @lastdate 22/02/2008
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <assert.h>

#include <net/iface.h>
#include <net/link.h>
//#include <net/prefix.h>
#include <net/net_types.h>
//#include <net/subnet.h>

// -----[ _net_iface_ptp_send ]--------------------------------------
static int _net_iface_ptp_send(net_addr_t tPhysAddr,
				  void * pContext,
				  SNetIface ** ppDstIface,
				  SNetMessage ** ppMsg)
{
  SNetIface * pIface= (SNetIface *) pContext;

  assert(pIface->tType == NET_IFACE_PTP);
  
  if (!net_iface_is_connected(pIface))
    return NET_ERROR_LINK_DOWN;
  if (!net_iface_is_enabled(pIface))
    return NET_ERROR_LINK_DOWN;

  *ppDstIface= pIface->tDest.pIface;

  return NET_SUCCESS;
}

// -----[ net_iface_new_ptp ]----------------------------------------
SNetIface * net_iface_new_ptp(SNetNode * pNode, SPrefix sPrefix)
{
  SNetIface * pIface= net_iface_new(pNode, NET_IFACE_PTP);
  pIface->tIfaceAddr= sPrefix.tNetwork;
  pIface->tIfaceMask= sPrefix.uMaskLen;
  pIface->pContext= pIface;
  pIface->fSend= _net_iface_ptp_send;
  return pIface;
}

