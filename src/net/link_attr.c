// ==================================================================
// @(#)link_attr.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 12/01/2007
// $Id: link_attr.c,v 1.3 2009-03-24 16:17:11 bqu Exp $
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <assert.h>
#include <stdlib.h>

#include <net/link.h>
#include <net/link_attr.h>

// -----[ net_igp_weights_create ]-----------------------------------
igp_weights_t * net_igp_weights_create(net_tos_t depth,
				       igp_weight_t dflt)
{
  unsigned int index;
  igp_weights_t * weights;
  assert((depth > 0) && (depth < NET_LINK_MAX_DEPTH));
  weights= (igp_weights_t *) uint32_array_create(depth);
  for (index= 0; index < depth; index++)
    weights->data[index]= dflt;
  return weights;
}

// -----[ net_igp_add_weights ]------------------------------------
/**
 * This is used to safely add two IGP weights in a larger
 * accumalutor in order to avoid overflow.
 */
igp_weight_t net_igp_add_weights(igp_weight_t weight1,
				 igp_weight_t weight2)
{
  uint64_t big_weight= weight1;
  big_weight+= weight2;
  if (big_weight > IGP_MAX_WEIGHT)
    return IGP_MAX_WEIGHT;
  return big_weight;
}
