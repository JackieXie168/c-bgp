// ==================================================================
// @(#)iface_ptmp.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 19/02/2008
// $Id: iface_ptmp.h,v 1.2 2008-04-07 09:31:46 bqu Exp $
// ==================================================================

#ifndef __NET_IFACE_PTMP_H__
#define __NET_IFACE_PTMP_H__

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ net_iface_new_ptmp ]--------------------------------------
  net_iface_t * net_iface_new_ptmp(net_node_t * pNode, SPrefix sPrefix);

#ifdef __cplusplus
}
#endif

#endif /* __NET_IFACE_PTMP_H__ */
