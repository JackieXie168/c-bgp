// ==================================================================
// @(#)attr.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 21/11/2005
// @lastdate 20/07/2007
// ==================================================================

#ifndef __BGP_ATTR_H__
#define __BGP_ATTR_H__

#include <bgp/comm.h>
#include <bgp/ecomm.h>
#include <bgp/path.h>
#include <bgp/types.h>

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ bgp_attr_create ]----------------------------------------
  SBGPAttr * bgp_attr_create(net_addr_t tNextHop,
			     bgp_origin_t tOrigin,
			     uint32_t uLocalPref,
			     uint32_t uMED);
  // -----[ bgp_attr_destroy ]---------------------------------------
  void bgp_attr_destroy(SBGPAttr ** ppAttr);
  // -----[ bgp_attr_set_nexthop ]-----------------------------------
  void bgp_attr_set_nexthop(SBGPAttr ** ppAttr, net_addr_t tNextHop);
  // -----[ bgp_attr_set_origin ]------------------------------------
  void bgp_attr_set_origin(SBGPAttr ** ppAttr, bgp_origin_t tOrigin);
  // -----[ bgp_attr_set_path ]--------------------------------------
  void bgp_attr_set_path(SBGPAttr ** ppAttr, SBGPPath * pPath);
  // -----[ bgp_attr_path_prepend ]----------------------------------
  int bgp_attr_path_prepend(SBGPAttr ** ppAttr, uint16_t uAS,
			    uint8_t uAmount);
  // -----[ bgp_attr_set_comm ]--------------------------------------
  void bgp_attr_set_comm(SBGPAttr ** ppAttr, SCommunities * pCommunities);
  // -----[ bgp_attr_comm_append ]-----------------------------------
  int bgp_attr_comm_append(SBGPAttr ** ppAttr, comm_t tCommunity);
  // -----[ bgp_attr_comm_remove ]-----------------------------------
  void bgp_attr_comm_remove(SBGPAttr ** ppAttr, comm_t tCommunity);
  // -----[ bgp_attr_comm_strip ]------------------------------------
  void bgp_attr_comm_strip(SBGPAttr ** ppAttr);
  // -----[ bgp_attr_ecomm_append ]----------------------------------
  int bgp_attr_ecomm_append(SBGPAttr ** ppAttr, SECommunity * pComm);
  // -----[ bgp_attr_originator_destroy ]----------------------------
  void bgp_attr_originator_destroy(SBGPAttr * pAttr);
  // -----[ bgp_attr_cluster_list_destroy ]--------------------------
  void bgp_attr_cluster_list_destroy(SBGPAttr * pAttr);
  
  // -----[ bgp_attr_cmp ]-------------------------------------------
  int bgp_attr_cmp(SBGPAttr * pAttr1, SBGPAttr * pAttr2);
  // -----[ bgp_attr_copy ]------------------------------------------
  SBGPAttr * bgp_attr_copy(SBGPAttr * pAttr);

#ifdef __cplusplus
}
#endif
  
#endif /* __BGP_ATTR_H__ */
