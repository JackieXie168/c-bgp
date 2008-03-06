// ==================================================================
// @(#)iface_ptp.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 19/02/2008
// @lastdate 19/02/2008
// ==================================================================

#ifndef __NET_IFACE_PTP_H__
#define __NET_IFACE_PTP_H__

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ net_iface_new_ptp ]--------------------------------------
  SNetIface * net_iface_new_ptp(SNetNode * pNode, SPrefix sPrefix);

#ifdef __cplusplus
}
#endif

#endif /* __NET_IFACE_PTP_H__ */
