// ==================================================================
// @(#)iface_ptmp.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 19/02/2008
// $Id: iface_ptmp.c,v 1.7 2009-08-31 09:48:28 bqu Exp $
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <assert.h>

#include <net/icmp_options.h>
#include <net/iface.h>
#include <net/link.h>
#include <net/network.h>
#include <net/prefix.h>
#include <net/net_types.h>
#include <net/subnet.h>

// -----[ _net_iface_ptmp_send ]-------------------------------------
static net_error_t _net_iface_ptmp_send(net_iface_t * self,
					net_addr_t l2_addr,
					net_msg_t * msg)
{
  net_subnet_t * subnet;
  net_iface_t * dst_iface;
  net_error_t error;
  int reached= 0;

  assert(self->type == NET_IFACE_PTMP);
  
  subnet= self->dest.subnet;

  error= ip_opt_hook_msg_subnet(subnet, msg, &reached);
  if (error != ESUCCESS)
    return error;
  if (reached) {
    message_destroy(&msg);
    return ESUCCESS;
  }

  // Find destination node (based on "physical address")
  dst_iface= net_subnet_find_link(subnet, l2_addr);
  if (dst_iface == NULL)
    return ENET_HOST_UNREACH;

  // Forward along link from subnet -> node ...
  if (!net_iface_is_enabled(dst_iface))
    return ENET_LINK_DOWN;

  network_send(dst_iface, msg);
  return ESUCCESS;
}

// -----[ net_iface_new_ptmp ]---------------------------------------
net_iface_t * net_iface_new_ptmp(net_node_t * node, ip_pfx_t pfx)
{
  net_iface_t * iface= net_iface_new(node, NET_IFACE_PTMP);
  iface->addr= pfx.network;
  iface->mask= pfx.mask;
  iface->ops.send= _net_iface_ptmp_send;
  return iface;
}



