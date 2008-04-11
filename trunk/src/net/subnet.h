// ==================================================================
// @(#)subnet.h
//
// @author Stefano Iasi (stefanoia@tin.it)
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 14/06/2005
// $Id: subnet.h,v 1.12 2008-04-11 11:03:07 bqu Exp $
// ==================================================================

#ifndef __NET_SUBNET_H__
#define __NET_SUBNET_H__

#include <libgds/types.h>
#include <libgds/array.h>
#include <libgds/log.h>
#include <net/net_types.h>
#include <net/prefix.h>

#ifdef __cplusplus
extern "C" {
#endif

  // ----- subnet_create --------------------------------------------
  net_subnet_t * subnet_create(net_addr_t network, uint8_t len,
			       net_subnet_type_t type);
  // ----- subnet_dump ----------------------------------------------
  void subnet_dump(SLogStream * stream, net_subnet_t * subnet);
  // ----- subnet_get_Prefix ----------------------------------------
  ip_pfx_t * subnet_get_prefix(net_subnet_t * subnet);
  // ----- subnet_is_transit ----------------------------------------
  int subnet_is_transit(net_subnet_t * subnet);
  // ----- subnet_is_stub -------------------------------------------
  int subnet_is_stub(net_subnet_t * subnet);
  // ----- subnet_add_link ------------------------------------------
  int subnet_add_link(net_subnet_t * subnet, net_iface_t * link);
  // ----- net_subnet_find_link -----------------------------------------
  net_iface_t * net_subnet_find_link(net_subnet_t * subnet,
				     net_addr_t dst_addr);
  // ----- subnet_get_links -----------------------------------------
  SPtrArray * subnet_get_links(net_subnet_t * subnet);
  // ----- subnet_destroy -------------------------------------------
  void subnet_destroy(net_subnet_t ** subnet_ref);
  
  int _subnet_test();
  
  // ----- _subnet_forward ------------------------------------------
  int _subnet_forward(net_addr_t phys_addr, void * ctx,
		      net_node_t ** next_hop_ref, net_msg_t ** msg_ref);


  ///////////////////////////////////////////////////////////////////
  // SUBNETS LIST FUNCTIONS
  ///////////////////////////////////////////////////////////////////

  // ----- subnets_create -------------------------------------------
  net_subnets_t * subnets_create();
  // ----- subnets_destroy ------------------------------------------
  void subnets_destroy(net_subnets_t ** subnets_ref);
  // ----- subnets_add ----------------------------------------------
  int subnets_add(net_subnets_t * subnets, net_subnet_t * subnet);
  // ----- subnets_find ---------------------------------------------
  net_subnet_t * subnets_find(net_subnets_t * subnets, ip_pfx_t prefix);

#ifdef __cplusplus
}
#endif

#endif /* __NET_SUBNET_H__ */


