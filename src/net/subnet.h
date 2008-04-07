// ==================================================================
// @(#)subnet.h
//
// @author Stefano Iasi (stefanoia@tin.it)
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 14/06/2005
// @lastdate 11/03/2008
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
  net_subnet_t * subnet_create(net_addr_t tNetwork, uint8_t uMaskLen,
			     uint8_t uType);
  // ----- subnet_dump ----------------------------------------------
  void subnet_dump(SLogStream * pStream, net_subnet_t * pSubnet);
  // ----- subnet_getAddr -------------------------------------------
  //net_addr_t subnet_get_Addr(net_subnet_t * pSubnet);
  // ----- subnet_get_Prefix ----------------------------------------
  SPrefix * subnet_get_prefix(net_subnet_t * pSubnet);
  // ----- subnet_is_transit ----------------------------------------
  int subnet_is_transit(net_subnet_t * pSubnet);
  // ----- subnet_is_stub -------------------------------------------
  int subnet_is_stub(net_subnet_t * pSubnet);
  // ----- subnet_add_link ------------------------------------------
  int subnet_add_link(net_subnet_t * pSubnet, net_iface_t * pLink);
  // ----- net_subnet_find_link -----------------------------------------
  net_iface_t * net_subnet_find_link(net_subnet_t * pSubnet,
				  net_addr_t tDstAddr);
  // ----- subnet_get_links -----------------------------------------
  SPtrArray * subnet_get_links(net_subnet_t * pSubnet);
  // ----- subnet_destroy -------------------------------------------
  void subnet_destroy(net_subnet_t ** ppSubnet);
  
  int _subnet_test();
  
  // ----- _subnet_forward ------------------------------------------
  int _subnet_forward(net_addr_t tPhysAddr, void * pContext,
		      net_node_t ** ppNextHop, net_msg_t ** ppMsg);


  ///////////////////////////////////////////////////////////////////
  // SUBNETS LIST FUNCTIONS
  ///////////////////////////////////////////////////////////////////

  // ----- subnets_create -------------------------------------------
  net_subnets_t * subnets_create();
  // ----- subnets_destroy ------------------------------------------
  void subnets_destroy(net_subnets_t ** ppSubnets);
  // ----- subnets_add ----------------------------------------------
  int subnets_add(net_subnets_t * pSubnets, net_subnet_t * pSubnet);
  // ----- subnets_find ---------------------------------------------
  net_subnet_t * subnets_find(net_subnets_t * pSubnets, SPrefix sPrefix);

#ifdef __cplusplus
}
#endif

#endif /* __NET_SUBNET_H__ */


