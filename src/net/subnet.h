// ==================================================================
// @(#)subnet.h
//
// @author Stefano Iasi (stefanoia@tin.it)
// @date 14/06/2005
// @lastdate 15/06/2005
// ==================================================================

#ifndef __NET_SUBNET_XYZ_H__
#define __NET_SUBNET_XYZ_H__

//#include <net/link.h>
#include <libgds/types.h>
#include <libgds/array.h>
#include <net/prefix.h>
#include <net/network_t.h>

#define NET_SUBNET_TYPE_TRANSIT 0
#define NET_SUBNET_TYPE_STUB    1

/*
  Describe transit network or stub network: if there are only one 
  router this object describe a stub network. A trasit network
  otherwise.
*/
typedef struct {
  net_addr_t tAddr;
  uint8_t uType;
  uint32_t uOSPFArea;	/*we have only an area for a network*/
  SPtrArray * aNodes;   /*links towards all the router on the subnet*/
} SNetSubnet;

// ----- subnet_create ---------------------------------------------------
SNetSubnet * subnet_create(net_addr_t tAddr, uint8_t uType);
// ----- subnet_getAddr --------------------------------------------------
net_addr_t subnet_get_Addr(SNetSubnet * pSubnet);
// ----- subnet_is_transit -----------------------------------------------
int subnet_is_transit(SNetSubnet * pSubnet);
// ----- subnet_is_stub -------------------------------------------------
int subnet_is_stub(SNetSubnet * pSubnet);
// ----- subnet_add_node -------------------------------------------------
int subnet_link_toNode(SNetSubnet * pSubnet, SNetNode * pNode); 

#endif


