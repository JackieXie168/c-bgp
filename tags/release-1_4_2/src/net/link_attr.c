// ==================================================================
// @(#)link_attr.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @lastdate 12/01/2007
// @date 16/01/2007
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <assert.h>
#include <stdlib.h>

#include <net/link.h>
#include <net/link_attr.h>

// -----[ net_igp_weights_create ]-----------------------------------
SNetIGPWeights * net_igp_weights_create(net_tos_t tDepth)
{
  unsigned int uIndex;
  SNetIGPWeights * pNew= (SNetIGPWeights *) uint32_array_create(0);
  assert((tDepth > 0) && (tDepth < NET_LINK_MAX_DEPTH));
  _array_set_length((SArray *) pNew, tDepth);
  for (uIndex= 0; uIndex < tDepth; uIndex++)
    pNew->data[uIndex]= UINT_MAX;
  return pNew;
}

// -----[ net_igp_weights_destroy ]----------------------------------
void net_igp_weights_destroy(SNetIGPWeights ** ppWeights)
{
  _array_destroy((SArray **) ppWeights);
}

// -----[ net_igp_weights_depth ]------------------------------------
unsigned int net_igp_weights_depth(SNetIGPWeights * pWeights)
{
  return _array_length((SArray *) pWeights);
}

// -----[ net_igp_add_weights ]------------------------------------
/**
 * This is used to safely add two IGP weights in a larger
 * accumalutor in order to avoid overflow.
 */
net_igp_weight_t net_igp_add_weights(net_igp_weight_t tWeight1,
				     net_igp_weight_t tWeight2)
{
  uint64_t tBigWeight= tWeight1;
  tBigWeight+= tWeight2;
  if (tBigWeight > NET_IGP_MAX_WEIGHT)
    return NET_IGP_MAX_WEIGHT;
  return tBigWeight;
}
