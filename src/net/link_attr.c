// ==================================================================
// @(#)link_attr.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @lastdate 12/01/2007
// $Id: link_attr.c,v 1.2 2008-04-11 11:03:06 bqu Exp $
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <assert.h>
#include <stdlib.h>

#include <net/link.h>
#include <net/link_attr.h>

// -----[ net_igp_weights_create ]-----------------------------------
igp_weights_t * net_igp_weights_create(net_tos_t depth)
{
  unsigned int index;
  igp_weights_t * weights= (igp_weights_t *) uint32_array_create(0);
  assert((depth > 0) && (depth < NET_LINK_MAX_DEPTH));
  _array_set_length((SArray *) weights, depth);
  for (index= 0; index < depth; index++)
    weights->data[index]= IGP_MAX_WEIGHT;
  return weights;
}

// -----[ net_igp_weights_destroy ]----------------------------------
void net_igp_weights_destroy(igp_weights_t ** weights_ref)
{
  _array_destroy((SArray **) weights_ref);
}

// -----[ net_igp_weights_depth ]------------------------------------
unsigned int net_igp_weights_depth(igp_weights_t * weights)
{
  return _array_length((SArray *) weights);
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
