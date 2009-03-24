// ==================================================================
// @(#) origin.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @data 18/12/2009
// $Id: origin.h,v 1.1 2009-03-24 13:41:26 bqu Exp $
// ==================================================================

#ifndef __BGP_ATTR_ORIGIN_H__
#define __BGP_ATTR_ORIGIN_H__

#include <bgp/types.h>

#ifdef _cplusplus
extern "C" {
#endif

  // -----[ bgp_origin_from_str ]------------------------------------
  int bgp_origin_from_str(const char * str, bgp_origin_t * origin_ref);

  // -----[ bgp_origin_to_str ]--------------------------------------
  const char * bgp_origin_to_str(bgp_origin_t origin);

#ifdef _cplusplus
}
#endif

#endif /* __BGP_ATTR_ORIGIN_H__ */
