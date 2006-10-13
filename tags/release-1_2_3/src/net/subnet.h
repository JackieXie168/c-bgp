// ==================================================================
// @(#)subnet.h
//
// @author Stefano Iasi (stefanoia@tin.it)
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 14/06/2005
// @lastdate 04/08/2005
// ==================================================================

#ifndef __NET_SUBNET_H__
#define __NET_SUBNET_H__

#include <libgds/types.h>
#include <libgds/array.h>
#include <net/net_types.h>
#include <net/prefix.h>
#include <net/network_t.h>

#define NET_SUBNET_TYPE_TRANSIT 0
#define NET_SUBNET_TYPE_STUB    1

// ----- subnet_create ----------------------------------------------
extern SNetSubnet * subnet_create(net_addr_t tNetwork, uint8_t uMaskLen,
				  uint8_t uType);
// ----- subnet_dump ------------------------------------------------
extern void subnet_dump(FILE * pStream, SNetSubnet * pSubnet);
// ----- subnet_getAddr ---------------------------------------------
//extern net_addr_t subnet_get_Addr(SNetSubnet * pSubnet);
// ----- subnet_get_Prefix ------------------------------------------
extern SPrefix * subnet_get_prefix(SNetSubnet * pSubnet);
// ----- subnet_is_transit ------------------------------------------
extern int subnet_is_transit(SNetSubnet * pSubnet);
// ----- subnet_is_stub ---------------------------------------------
extern int subnet_is_stub(SNetSubnet * pSubnet);
// ----- subnet_add_link --------------------------------------------
extern int subnet_add_link(SNetSubnet * pSubnet, SNetLink * pLink,
			   net_addr_t tIfaceAddr);
// ----- subnet_get_links -------------------------------------------
extern SPtrArray * subnet_get_links(SNetSubnet * pSubnet);
// ----- subnet_destroy ---------------------------------------------
extern void subnet_destroy(SNetSubnet ** ppSubnet);
// ----- subnets_compare -----------------------------------------
extern int subnets_compare(void * pItem1, void * pItem2,
		       unsigned int uEltSize);

extern int _subnet_test();

// ----- _subnet_forward --------------------------------------------
extern int _subnet_forward(net_addr_t tDstAddr, void * pContext,
			   SNetNode ** ppNextHop);

#endif /* __NET_SUBNET_H__ */


