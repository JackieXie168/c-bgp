// ==================================================================
// @(#)iface_rtr.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 19/02/2008
// $Id: iface_rtr.h,v 1.3 2008-06-13 14:26:23 bqu Exp $
// ==================================================================

#ifndef __NET_IFACE_RTR_H__
#define __NET_IFACE_RTR_H__

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ net_iface_new_rtr ]--------------------------------------
  net_iface_t * net_iface_new_rtr(net_node_t * node, net_addr_t addr);

#ifdef __cplusplus
}
#endif

#endif /* __NET_IFACE_RTR_H__ */
