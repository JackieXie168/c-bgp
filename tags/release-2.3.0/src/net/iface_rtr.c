// ==================================================================
// @(#)iface_rtr.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 19/02/2008
// $Id: iface_rtr.c,v 1.4 2009-03-24 16:11:45 bqu Exp $
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <net/iface.h>
#include <net/link.h>
#include <net/network.h>
#include <net/prefix.h>
#include <net/net_types.h>

// -----[ _net_iface_rtr_send ]--------------------------------------
/**
 * Forward a message along a point-to-point link. The next-hop
 * information is not used.
 */
static int _net_iface_rtr_send(net_iface_t * self,
			       net_addr_t l2_addr,
			       net_msg_t * msg)
{
  network_send(self->dest.iface, msg);
  return ESUCCESS;
}

// -----[ net_iface_new_rtr ]----------------------------------------
net_iface_t * net_iface_new_rtr(net_node_t * node, net_addr_t addr)
{
  net_iface_t * iface= net_iface_new(node, NET_IFACE_RTR);
  iface->addr= addr;
  iface->mask= 32;
  iface->ops.send= _net_iface_rtr_send;
  return iface;
}
