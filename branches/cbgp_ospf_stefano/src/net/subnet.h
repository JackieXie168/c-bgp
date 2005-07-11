// ==================================================================
// @(#)subnet.h
//
// @author Stefano Iasi (stefanoia@tin.it)
// @date 14/06/2005
// @lastdate 15/06/2005
// ==================================================================

#ifndef __NET_SUBNET_XYZ_H__
#define __NET_SUBNET_XYZ_H__

#include <libgds/types.h>
#include <libgds/array.h>
#include <net/net_types.h>
#include <net/prefix.h>
#include <net/network_t.h>


#define NET_SUBNET_TYPE_TRANSIT 0
#define NET_SUBNET_TYPE_STUB    1



// ----- subnet_create -----------------------------------------------------------------
//SNetSubnet * subnet_create(/*net_addr_t tAddr*/SPrefix * pPrefix, uint8_t uType);
// ----- subnet_create -----------------------------------------------------------------
SNetSubnet * subnet_create(net_addr_t tNetwork, uint8_t uMaskLen, uint8_t uType);
// --------- subnet_dump ---------------------------------------------------------------
void subnet_dump(FILE * pStream, SNetSubnet * pSubnet);
// ----- subnet_getAddr ----------------------------------------------------------------
//net_addr_t subnet_get_Addr(SNetSubnet * pSubnet);
// ----- subnet_get_Prefix -------------------------------------------------------------
SPrefix * subnet_get_prefix(SNetSubnet * pSubnet);
// ----- subnet_is_transit -------------------------------------------------------------
int subnet_is_transit(SNetSubnet * pSubnet);
// ----- subnet_is_stub ----------------------------------------------------------------
int subnet_is_stub(SNetSubnet * pSubnet);
// ----- subnet_add_node ---------------------------------------------------------------
int subnet_link_toNode(SNetSubnet * pSubnet, SNetNode * pNode); 
// ----- subnet_get_links -------------------------------------------------------------
SPtrArray * subnet_get_links(SNetSubnet * pSubnet);
// ----- subnet_destroy ---------------------------------------------------
void subnet_destroy(SNetSubnet ** ppSubnet);
// ----- subnets_compare -----------------------------------------
int subnets_compare(void * pItem1, void * pItem2,
		       unsigned int uEltSize);
int subnet_test();
#endif


