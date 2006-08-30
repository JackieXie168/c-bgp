// ==================================================================
// @(#)attr.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 21/11/2005
// @lastdate 21/11/2005
// ==================================================================

#ifndef __BGP_ATTR_H__
#define __BGP_ATTR_H__

#include <bgp/comm.h>
#include <bgp/ecomm.h>
#include <bgp/path.h>
#include <bgp/types.h>

// -----[ bgp_attr_create ]------------------------------------------
extern SBGPAttr * bgp_attr_create(net_addr_t tNextHop,
				  bgp_origin_t tOrigin,
				  uint32_t uLocalPref,
				  uint32_t uMED);
// -----[ bgp_attr_destroy ]-----------------------------------------
extern void bgp_attr_destroy(SBGPAttr ** ppAttr);
// -----[ bgp_attr_set_nexthop ]-------------------------------------
extern inline void bgp_attr_set_nexthop(SBGPAttr ** ppAttr,
					net_addr_t tNextHop);
// -----[ bgp_attr_set_origin ]-------------------------------------
extern inline void bgp_attr_set_origin(SBGPAttr ** ppAttr,
				       bgp_origin_t tOrigin);
// -----[ bgp_attr_set_path ]----------------------------------------
extern inline void bgp_attr_set_path(SBGPAttr ** ppAttr,
				     SBGPPath * pPath);
// -----[ bgp_attr_path_prepend ]------------------------------------
extern int bgp_attr_path_prepend(SBGPAttr ** ppAttr, uint16_t uAS,
				  uint8_t uAmount);
// -----[ bgp_attr_set_comm ]----------------------------------------
extern inline void bgp_attr_set_comm(SBGPAttr ** ppAttr,
				     SCommunities * pCommunities);
// -----[ bgp_attr_comm_destroy ]---------------------------------------
extern inline void bgp_attr_comm_destroy(SBGPAttr ** ppAttr);
// -----[ bgp_attr_comm_remove ]-------------------------------------
extern inline void bgp_attr_comm_remove(SBGPAttr ** ppAttr, comm_t tCommunity);
// -----[ bgp_attr_cmp ]---------------------------------------------
extern int bgp_attr_cmp(SBGPAttr * pAttr1, SBGPAttr * pAttr2);
// -----[ bgp_attr_copy ]--------------------------------------------
extern SBGPAttr * bgp_attr_copy(SBGPAttr * pAttr);

#endif /* __BGP_ATTR_H__ */
