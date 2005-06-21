// ==================================================================
// @(#)subnet.c
//
// @author Stefano Iasi (stefanoia@tin.it)
// @date 14/06/2005
// @lastdate 14/05/2005
// ==================================================================

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <libgds/memory.h>
#include <net/subnet.h>
#include <net/ospf.h>
#include <net/network.h>


/////////////////////////////////////////////////////////////////////
// SNetSubnet Methods
/////////////////////////////////////////////////////////////////////

// ----- subnet_create ---------------------------------------------------
SNetSubnet * subnet_create(/*net_addr_t tAddr*/SPrefix * pPrefix, uint8_t uType)
{
  SNetSubnet * pSubnet= (SNetSubnet *) MALLOC(sizeof(SNetSubnet));
  pSubnet->pPrefix= pPrefix;//tAddr;
  pSubnet->uOSPFArea = NO_AREA;
  pSubnet->uType = uType;
  
  pSubnet->aNodes = ptr_array_create(ARRAY_OPTION_SORTED|ARRAY_OPTION_UNIQUE,
				  node_compare,
				  NULL);
  return pSubnet; 
}

SNetSubnet * subnet_create_byAddr(net_addr_t tNetwork, uint8_t uMaskLen, uint8_t uType)
{
  SNetSubnet * pSubnet= (SNetSubnet *) MALLOC(sizeof(SNetSubnet));
  SPrefix * pPrefix = (SPrefix *) MALLOC(sizeof(SPrefix));
  pPrefix->tNetwork = tNetwork;
  pPrefix->uMaskLen = uMaskLen;
  
  pSubnet->pPrefix= pPrefix;//tAddr;
  pSubnet->uOSPFArea = NO_AREA;
  pSubnet->uType = uType;
  
  pSubnet->aNodes = ptr_array_create(ARRAY_OPTION_SORTED|ARRAY_OPTION_UNIQUE,
				  node_compare,
				  NULL);
  return pSubnet; 
}

// ----- subnet_link_to_node ----------------------------------------
//WARNING: this should be invoke only by node_add_link_toSubnet
int subnet_link_toNode(SNetSubnet * pSubnet, SNetNode * pNode){
  return ptr_array_add(pSubnet->aNodes, &pNode);
  /*this is to dinamically set transit node... for now we require that 
    transit node are known at start time
    this can be interesting to model link failure that cause a transformation from
    stub network to transit network
    */
  /*  if (subnet_is_transit(pSubnet) && pLink->uDestinationType != NET_LINK_TYPE_TRANSIT){ //count number of router on the network
     SNetLink * pLink = node_find_link(SNetNode * pNode, net_addr_t tAddr);
     if (pLink == NULL)
       return 0;
     else {
       pLink->uDestinationType = NET_LINK_TYPE_TRANSIT;
       return 1;
     }
    }
  return 0;*/
}



// ----- subnet_getAddr --------------------------------------------------
/*net_addr_t subnet_get_Addr(SNetSubnet * pSubnet)
{
  return pSubnet->tAddr;
}*/

// ----- subnet_getAddr --------------------------------------------------
SPrefix * subnet_get_Prefix(SNetSubnet * pSubnet)
{
  return pSubnet->pPrefix;
}

// ----- subnet_is_transit -------------------------------------------------
int subnet_is_transit(SNetSubnet * pSubnet) {
  return (pSubnet->uType == NET_SUBNET_TYPE_TRANSIT);
}

// ----- subnet_is_stub -------------------------------------------------
int subnet_is_stub(SNetSubnet * pSubnet) {
  return (pSubnet->uType == NET_SUBNET_TYPE_STUB);
}
