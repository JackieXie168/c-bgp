// ==================================================================
// @(#)subnet.h
//
// @author Stefano Iasi (stefanoia@tin.it)
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 14/06/2005
// @lastdate 23/07/2007
// ==================================================================

#ifndef __NET_SUBNET_H__
#define __NET_SUBNET_H__

#include <libgds/types.h>
#include <libgds/array.h>
#include <libgds/log.h>
#include <net/net_types.h>
#include <net/prefix.h>

#define NET_SUBNET_TYPE_TRANSIT 0
#define NET_SUBNET_TYPE_STUB    1

#ifdef __cplusplus
extern "C" {
#endif

  // ----- subnet_create --------------------------------------------
  SNetSubnet * subnet_create(net_addr_t tNetwork, uint8_t uMaskLen,
			     uint8_t uType);
  // ----- subnet_dump ----------------------------------------------
  void subnet_dump(SLogStream * pStream, SNetSubnet * pSubnet);
  // ----- subnet_getAddr -------------------------------------------
  //net_addr_t subnet_get_Addr(SNetSubnet * pSubnet);
  // ----- subnet_get_Prefix ----------------------------------------
  SPrefix * subnet_get_prefix(SNetSubnet * pSubnet);
  // ----- subnet_is_transit ----------------------------------------
  int subnet_is_transit(SNetSubnet * pSubnet);
  // ----- subnet_is_stub -------------------------------------------
  int subnet_is_stub(SNetSubnet * pSubnet);
  // ----- subnet_add_link ------------------------------------------
  int subnet_add_link(SNetSubnet * pSubnet, SNetLink * pLink);
  // ----- subnet_find_link -----------------------------------------
  SNetLink * subnet_find_link(SNetSubnet * pSubnet, net_addr_t tDstAddr);
  // ----- subnet_get_links -----------------------------------------
  SPtrArray * subnet_get_links(SNetSubnet * pSubnet);
  // ----- subnet_destroy -------------------------------------------
  void subnet_destroy(SNetSubnet ** ppSubnet);
  
  int _subnet_test();
  
  // ----- _subnet_forward ------------------------------------------
  int _subnet_forward(net_addr_t tPhysAddr, void * pContext,
		      SNetNode ** ppNextHop, SNetMessage ** ppMsg);


  ///////////////////////////////////////////////////////////////////
  // SUBNETS LIST FUNCTIONS
  ///////////////////////////////////////////////////////////////////

  // ----- subnets_create -------------------------------------------
  SNetSubnets * subnets_create();
  // ----- subnets_destroy ------------------------------------------
  void subnets_destroy(SNetSubnets ** ppSubnets);
  // ----- subnets_add ----------------------------------------------
  int subnets_add(SNetSubnets * pSubnets, SNetSubnet * pSubnet);
  // ----- subnets_find ---------------------------------------------
  SNetSubnet * subnets_find(SNetSubnets * pSubnets, SPrefix sPrefix);

#ifdef __cplusplus
}
#endif

#endif /* __NET_SUBNET_H__ */


