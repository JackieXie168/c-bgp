// ==================================================================
// @(#)link_attr.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 12/01/2007
// @lastdate 22/01/2007
// ==================================================================

#ifndef __NET_LINK_ATTR_H__
#define __NET_LINK_ATTR_H__

#include <libgds/array.h>
#include <net/net_types.h>

typedef uint32_t net_igp_weight_t;
typedef SUInt32Array SNetIGPWeights;

#define NET_IGP_MAX_WEIGHT UINT32_MAX

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ net_igp_weights_create ]---------------------------------
  SNetIGPWeights * net_igp_weights_create(uint8_t tDepth);
  // -----[ net_igp_weights_destroy ]--------------------------------
  void net_igp_weights_destroy(SNetIGPWeights ** ppWeights);
  // -----[ net_igp_weights_depth ]----------------------------------
  unsigned int net_igp_weights_depth(SNetIGPWeights * pWeights);
  // -----[ net_igp_add_weights ]------------------------------------
  net_igp_weight_t net_igp_add_weights(net_igp_weight_t tWeight1,
				       net_igp_weight_t tWeight2);

#ifdef __cplusplus
}
#endif

#endif /* __NET_LINK_ATTR_H__ */
