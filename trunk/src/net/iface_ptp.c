// ==================================================================
// @(#)iface_ptp.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 19/02/2008
// $Id: iface_ptp.c,v 1.4 2009-03-24 16:11:45 bqu Exp $
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <assert.h>

#include <net/iface.h>
#include <net/link.h>
#include <net/net_types.h>
#include <net/network.h>

// -----[ _net_iface_ptp_send ]--------------------------------------
static int _net_iface_ptp_send(net_iface_t * self,
			       net_addr_t l2_addr,
			       net_msg_t * msg)
{
  assert(self->type == NET_IFACE_PTP);
  
  network_send(self->dest.iface, msg);
  return ESUCCESS;
}

// -----[ net_iface_new_ptp ]----------------------------------------
net_iface_t * net_iface_new_ptp(net_node_t * node, ip_pfx_t pfx)
{
  net_iface_t * iface= net_iface_new(node, NET_IFACE_PTP);
  iface->addr= pfx.network;
  iface->mask= pfx.mask;
  iface->ops.send= _net_iface_ptp_send;
  return iface;
}

