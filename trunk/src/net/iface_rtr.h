// ==================================================================
// @(#)iface_rtr.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 19/02/2008
// @lastdate 19/02/2008
// ==================================================================

#ifndef __NET_IFACE_RTR_H__
#define __NET_IFACE_RTR_H__

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ net_iface_new_rtr ]--------------------------------------
  SNetIface * net_iface_new_rtr(SNetNode * pNode, net_addr_t tAddr);

#ifdef __cplusplus
}
#endif

#endif /* __NET_IFACE_RTR_H__ */
