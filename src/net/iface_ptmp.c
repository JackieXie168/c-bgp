// ==================================================================
// @(#)iface_ptmp.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 19/02/2008
// $Id: iface_ptmp.c,v 1.5 2008-06-13 14:26:23 bqu Exp $
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

  assert(self->type == NET_IFACE_PTMP);
  
  subnet= self->dest.subnet;

  if ((msg != NULL) && (msg->protocol == NET_PROTOCOL_ICMP))
    if ((error= icmp_process_options(ICMP_OPT_STATE_SUBNET,
				     (net_node_t *) subnet,
				     self, msg, NULL)) != ESUCCESS)
      return error;


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
  iface->tIfaceAddr= pfx.tNetwork;
  iface->tIfaceMask= pfx.uMaskLen;
  iface->ops.send= _net_iface_ptmp_send;
  return iface;
}



