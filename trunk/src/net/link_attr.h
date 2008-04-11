// ==================================================================
// @(#)link_attr.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 12/01/2007
// $Id: link_attr.h,v 1.2 2008-04-11 11:03:06 bqu Exp $
// ==================================================================

#ifndef __NET_LINK_ATTR_H__
#define __NET_LINK_ATTR_H__

#include <libgds/array.h>
#include <net/net_types.h>

typedef uint32_t igp_weight_t;
typedef SUInt32Array igp_weights_t;

#define IGP_MAX_WEIGHT UINT32_MAX

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ net_igp_weights_create ]---------------------------------
  igp_weights_t * net_igp_weights_create(uint8_t depth);
  // -----[ net_igp_weights_destroy ]--------------------------------
  void net_igp_weights_destroy(igp_weights_t ** weights_ref);
  // -----[ net_igp_weights_depth ]----------------------------------
  unsigned int net_igp_weights_depth(igp_weights_t * weights);
  // -----[ net_igp_add_weights ]------------------------------------
  igp_weight_t net_igp_add_weights(igp_weight_t weight1,
				   igp_weight_t weight2);

#ifdef __cplusplus
}
#endif

#endif /* __NET_LINK_ATTR_H__ */
