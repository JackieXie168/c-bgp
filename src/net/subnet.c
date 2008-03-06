// ==================================================================
// @(#)subnet.c
//
// @author Stefano Iasi (stefanoia@tin.it)
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 14/06/2005
// @lastdate 18/02/2008
// ==================================================================

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <string.h>

#include <libgds/memory.h>

#include <net/error.h>
#include <net/link-list.h>
#include <net/node.h>
#include <net/subnet.h>
#include <net/ospf.h>
#include <net/network.h>


/////////////////////////////////////////////////////////////////////
//
// SNetSubnet Methods
//
/////////////////////////////////////////////////////////////////////

// ----- _net_subnet_link_compare -----------------------------------
static int _net_subnet_link_compare(void * pItem1, void * pItem2,
				    unsigned int uEltSize)
{
  SNetLink * pLink1= *((SNetLink **) pItem1);
  SNetLink * pLink2= *((SNetLink **) pItem2);

  if (pLink1->tIfaceAddr > pLink2->tIfaceAddr)
    return 1;
  if (pLink1->tIfaceAddr < pLink2->tIfaceAddr)
    return -1;
  
  return 0;
}

// ----- subnet_create ----------------------------------------------
SNetSubnet * subnet_create(net_addr_t tNetwork, uint8_t uMaskLen,
			   uint8_t uType)
{
  SNetSubnet * pSubnet= (SNetSubnet *) MALLOC(sizeof(SNetSubnet));
  assert(uMaskLen < 32); // subnets with a mask of /32 are not allowed
  pSubnet->sPrefix.tNetwork= tNetwork;
  pSubnet->sPrefix.uMaskLen= uMaskLen;
  ip_prefix_mask(&pSubnet->sPrefix);
  pSubnet->uType = uType;
  
#ifdef OSPF_SUPPORT
  pSubnet->uOSPFArea = OSPF_NO_AREA;
#endif

  // Create an array of references to links
  pSubnet->pLinks= ptr_array_create(ARRAY_OPTION_SORTED |
				    ARRAY_OPTION_UNIQUE,
				    _net_subnet_link_compare,
				    NULL);
  return pSubnet; 
}

// ----- subnet_destroy ---------------------------------------------
void subnet_destroy(SNetSubnet ** ppSubnet)
{
  if (*ppSubnet != NULL) {
    net_links_destroy(&(*ppSubnet)->pLinks);
    FREE(*ppSubnet);
    *ppSubnet= NULL;
  }
}

// ----- subnet_dump ------------------------------------------------
void subnet_dump(SLogStream * pStream, SNetSubnet * pSubnet)
{
  SNetLink * pLink = NULL;
  int iIndex;
  
  log_printf(pStream, "SUBNET PREFIX <");
  ip_prefix_dump(pStream, pSubnet->sPrefix);
#ifdef OSPF_SUPPORT
  log_printf(pStream, ">\tOSPF AREA  <%u>\t",
	     (unsigned int) pSubnet->uOSPFArea);
#endif
  switch (pSubnet->uType) {
  case NET_SUBNET_TYPE_TRANSIT : 
    log_printf(pStream, "TRANSIT\t");
    break;
  case NET_SUBNET_TYPE_STUB : 
    log_printf(pStream, "STUB\t");
    break;
  default:
    log_printf(pStream, "???\t");
  }
  log_printf(pStream, "LINKED TO: \t");
  //dump links 
  for (iIndex = 0; iIndex < ptr_array_length(pSubnet->pLinks); iIndex++) {
    ptr_array_get_at(pSubnet->pLinks, iIndex, &pLink);
    assert(pLink != NULL);
    ip_address_dump(pStream, pLink->tIfaceAddr);
    log_printf(pStream, "\n---\t\t\t\t\t\t");
  }
  log_printf(pStream,"\n");
}

// -----[ subnet_add_link ]------------------------------------------
int subnet_add_link(SNetSubnet * pSubnet, SNetLink * pLink)
{
  if (ptr_array_add(pSubnet->pLinks, &pLink) < 0) {
    return NET_ERROR_MGMT_LINK_ALREADY_EXISTS;
  }

  return NET_SUCCESS;
}

// ----- subnet_getAddr ---------------------------------------------
SPrefix * subnet_get_prefix(SNetSubnet * pSubnet) {
  return &(pSubnet->sPrefix);
}

// ----- subnet_is_transit ------------------------------------------
int subnet_is_transit(SNetSubnet * pSubnet) {
  return (pSubnet->uType == NET_SUBNET_TYPE_TRANSIT);
}

// ----- subnet_is_stub ---------------------------------------------
int subnet_is_stub(SNetSubnet * pSubnet) {
  return (pSubnet->uType == NET_SUBNET_TYPE_STUB);
}

// ----- subnet_find_link -------------------------------------------
/**
 * Find a link based on its interface address.
 */
SNetLink * net_subnet_find_link(SNetSubnet * pSubnet, net_addr_t tDstAddr)
{
  SNetLink sWrapLink= { .tIfaceAddr= tDstAddr };
  SNetLink * pLink= NULL;
  unsigned int uIndex;
  SNetLink * pWrapLink= &sWrapLink;

  if (ptr_array_sorted_find_index(pSubnet->pLinks, &pWrapLink, &uIndex) == 0)
    pLink= (SNetLink *) pSubnet->pLinks->data[uIndex];
  
  return pLink;
}


/////////////////////////////////////////////////////////////////////
//
// TESTS
//
/////////////////////////////////////////////////////////////////////

int _subnet_test(){
#ifdef OSPF_SUPPORT
  char * pcEndPtr;
  SPrefix  sSubnetPfxTx;
  SPrefix  sSubnetPfxTx1;
  net_addr_t tAddrA, tAddrB, tAddrC;
  
  LOG_DEBUG("subnet_test(): START\n");
  
  assert(!ip_string_to_address("192.168.0.2", &pcEndPtr, &tAddrA));
  assert(!ip_string_to_address("192.168.0.3", &pcEndPtr, &tAddrB));
  assert(!ip_string_to_address("192.168.0.4", &pcEndPtr, &tAddrC));
  
  LOG_DEBUG("subnet_test(): CHECK subnet methods... \n");
  SNetNode * pNodeA= node_create(tAddrA);
  SNetNode * pNodeB= node_create(tAddrB);
  SNetNode * pNodeC= node_create(tAddrC);
  
  assert(!ip_string_to_prefix("192.168.0.0/24", &pcEndPtr, &sSubnetPfxTx));
  assert(!ip_string_to_prefix("192.120.0.0/24", &pcEndPtr, &sSubnetPfxTx1));
  
  //ip_prefix_dump(stdout, sSubnetPfxTx);
  //ip_prefix_dump(stdout, sSubnetPfxTx1);
  
  SNetSubnet * pSubTx = subnet_create(sSubnetPfxTx.tNetwork,
				      sSubnetPfxTx.uMaskLen, NET_SUBNET_TYPE_TRANSIT);
  SNetSubnet * pSubTx1 = subnet_create(sSubnetPfxTx1.tNetwork,
				       sSubnetPfxTx.uMaskLen,
				       NET_SUBNET_TYPE_TRANSIT);
  //subnet_dump(stdout, pSubTx);
  //subnet_dump(stdout, pSubTx1);a
  net_addr_t ipIfaceA = IPV4_TO_INT(192,168,0,1);
  net_addr_t ipIfaceB = IPV4_TO_INT(192,168,0,2); 
  net_addr_t ipIfaceC = IPV4_TO_INT(192,168,0,3);
  assert(node_add_link_to_subnet(pNodeA, pSubTx, ipIfaceA, 100, 1) == NET_SUCCESS);
  assert(node_add_link_to_subnet(pNodeB, pSubTx, ipIfaceB, 100, 1) == NET_SUCCESS);
  assert(node_add_link_to_subnet(pNodeC, pSubTx, ipIfaceC, 100, 1) == NET_SUCCESS);
  assert(node_add_link_to_router(pNodeA, pNodeC, 100, 1) == NET_SUCCESS);
  
  
  SNetLink * pLinkAC = node_find_link_to_router(pNodeA, tAddrC);
  assert(link_get_address(pLinkAC) == (tAddrC));
  
  SNetLink * pLinkAB = node_find_link_to_router(pNodeA, tAddrB);
  assert(pLinkAB == NULL);
  
  SNetLink * pLinkAS = node_find_link_to_subnet(pNodeA, pSubTx, ipIfaceA);
  assert(pLinkAS != NULL);
  assert(link_get_address(pLinkAS) == ((pSubTx->sPrefix).tNetwork));

  SNetLink * pLinkAS1 = node_find_link_to_subnet(pNodeA, pSubTx1, ipIfaceB);
  assert(pLinkAS1 == NULL);
  
  subnet_destroy(&pSubTx);
  subnet_destroy(&pSubTx1);
  
  node_destroy(&pNodeA);
  node_destroy(&pNodeB);
  node_destroy(&pNodeC);
  
  LOG_DEBUG("subnet_test(): TEST on a bigger network... \n");
  /* other test on subnet methods... */
  const int startAddr = 1024;
  
  //SNetwork * pNetwork= network_create();
  SNetNode * pNodeB1= node_create(startAddr);
  SNetNode * pNodeB2= node_create(startAddr + 1);
  SNetNode * pNodeB3= node_create(startAddr + 2);
  SNetNode * pNodeB4= node_create(startAddr + 3);
  SNetNode * pNodeX1= node_create(startAddr + 4);
  SNetNode * pNodeX2= node_create(startAddr + 5);
  SNetNode * pNodeX3= node_create(startAddr + 6);
  SNetNode * pNodeY1= node_create(startAddr + 7);
  SNetNode * pNodeY2= node_create(startAddr + 8);
  SNetNode * pNodeK1= node_create(startAddr + 9);
  SNetNode * pNodeK2= node_create(startAddr + 10);
  SNetNode * pNodeK3= node_create(startAddr + 11);
  
  SNetSubnet * pSubnetTB1= subnet_create(4, 30, NET_SUBNET_TYPE_TRANSIT);
  SNetSubnet * pSubnetTX1= subnet_create(8, 30, NET_SUBNET_TYPE_TRANSIT);
  SNetSubnet * pSubnetTY1= subnet_create(12, 30, NET_SUBNET_TYPE_TRANSIT);
  SNetSubnet * pSubnetTK1= subnet_create(16, 30, NET_SUBNET_TYPE_TRANSIT);
  
  SNetSubnet * pSubnetSB1= subnet_create(20, 30, NET_SUBNET_TYPE_STUB);
  SNetSubnet * pSubnetSB2= subnet_create(24, 30, NET_SUBNET_TYPE_STUB);
  SNetSubnet * pSubnetSX1= subnet_create(28, 30, NET_SUBNET_TYPE_STUB);
  SNetSubnet * pSubnetSX2= subnet_create(32, 30, NET_SUBNET_TYPE_STUB);
  SNetSubnet * pSubnetSX3= subnet_create(36, 30, NET_SUBNET_TYPE_STUB);
  SNetSubnet * pSubnetSY1= subnet_create(40, 30, NET_SUBNET_TYPE_STUB);
  SNetSubnet * pSubnetSY2= subnet_create(44, 30, NET_SUBNET_TYPE_STUB);
  SNetSubnet * pSubnetSY3= subnet_create(48, 30, NET_SUBNET_TYPE_STUB);
  SNetSubnet * pSubnetSK1= subnet_create(52, 30, NET_SUBNET_TYPE_STUB);
  SNetSubnet * pSubnetSK2= subnet_create(56, 30, NET_SUBNET_TYPE_STUB);
  SNetSubnet * pSubnetSK3= subnet_create(60, 30, NET_SUBNET_TYPE_STUB);

  assert(!network_add_node(pNodeB1));
  assert(!network_add_node(pNodeB2));
  assert(!network_add_node(pNodeB3));
  assert(!network_add_node(pNodeB4));
  assert(!network_add_node(pNodeX1));
  assert(!network_add_node(pNodeX2));
  assert(!network_add_node(pNodeX3));
  assert(!network_add_node(pNodeY1));
  assert(!network_add_node(pNodeY2));
  assert(!network_add_node(pNodeK1));
  assert(!network_add_node(pNodeK2));
  assert(!network_add_node(pNodeK3));
  
  assert(network_add_subnet(pSubnetTB1) >= 0);
  assert(network_add_subnet(pSubnetTX1) >= 0);
  assert(network_add_subnet(pSubnetTY1) >= 0);
  assert(network_add_subnet(pSubnetTK1) >= 0);
  
  assert(network_add_subnet(pSubnetSB1) >= 0);
  assert(network_add_subnet(pSubnetSB2) >= 0);
  assert(network_add_subnet(pSubnetSX1) >= 0);
  assert(network_add_subnet(pSubnetSX2) >= 0);
  assert(network_add_subnet(pSubnetSX3) >= 0);
  assert(network_add_subnet(pSubnetSY1) >= 0);
  assert(network_add_subnet(pSubnetSY2) >= 0);
  assert(network_add_subnet(pSubnetSY3) >= 0);
  assert(network_add_subnet(pSubnetSK1) >= 0);
  assert(network_add_subnet(pSubnetSK2) >= 0);
  assert(network_add_subnet(pSubnetSK3) >= 0);
  
  
//   LOG_DEBUG("nodes attached.\n");

  assert(node_add_link_to_router(pNodeB1, pNodeB2, 100, 1) >= 0);
  assert(node_add_link_to_router(pNodeB2, pNodeB3, 100, 1) >= 0);
  assert(node_add_link_to_router(pNodeB3, pNodeB4, 100, 1) >= 0);
  assert(node_add_link_to_router(pNodeB4, pNodeB1, 100, 1) >= 0);
  
  assert(node_add_link_to_router(pNodeB2, pNodeX2, 100, 1) >= 0);
  assert(node_add_link_to_router(pNodeB3, pNodeX2, 100, 1) >= 0);
  assert(node_add_link_to_router(pNodeX2, pNodeX1, 100, 1) >= 0);
  assert(node_add_link_to_router(pNodeX2, pNodeX3, 100, 1) >= 0);

  assert(node_add_link_to_router(pNodeB3, pNodeY1, 100, 1) >= 0);
  assert(node_add_link_to_router(pNodeY1, pNodeY2, 100, 1) >= 0);
  
  assert(node_add_link_to_router(pNodeB4, pNodeK1, 100, 1) >= 0);
  assert(node_add_link_to_router(pNodeB4, pNodeK2, 100, 1) >= 0);
  assert(node_add_link_to_router(pNodeB1, pNodeK3, 100, 1) >= 0);
  assert(node_add_link_to_router(pNodeK3, pNodeK2, 100, 1) >= 0);
  
//   LOG_DEBUG("point-to-point links attached.\n");
  
  assert(node_add_link_to_subnet(pNodeB1, pSubnetTB1, 1, 100, 1) >= 0);
  assert(node_add_link_to_subnet(pNodeB3, pSubnetTB1, 2, 100, 1) >= 0);
  assert(node_add_link_to_subnet(pNodeX1, pSubnetTX1, 3, 100, 1) >= 0);
  assert(node_add_link_to_subnet(pNodeX3, pSubnetTX1, 4, 100, 1) >= 0);
  assert(node_add_link_to_subnet(pNodeB3, pSubnetTY1, 5, 100, 1) >= 0);
  assert(node_add_link_to_subnet(pNodeY2, pSubnetTY1, 6, 100, 1) >= 0);
  assert(node_add_link_to_subnet(pNodeK2, pSubnetTK1, 7, 100, 1) >= 0);
  assert(node_add_link_to_subnet(pNodeK1, pSubnetTK1, 8, 100, 1) >= 0);
 
//   LOG_DEBUG("transit-network links attached.\n");
  
  assert(node_add_link_to_subnet(pNodeB2, pSubnetSB1, 9, 100, 1) >= 0);
  assert(node_add_link_to_subnet(pNodeB1, pSubnetSB2, 10, 100, 1) >= 0);
  assert(node_add_link_to_subnet(pNodeB2, pSubnetSX1, 11, 100, 1) >= 0);
  assert(node_add_link_to_subnet(pNodeX2, pSubnetSX2, 12, 100, 1) >= 0);
  assert(node_add_link_to_subnet(pNodeX3, pSubnetSX3, 13, 100, 1) >= 0);
  assert(node_add_link_to_subnet(pNodeB3, pSubnetSY1, 14, 100, 1) >= 0);
  assert(node_add_link_to_subnet(pNodeY1, pSubnetSY2, 15, 100, 1) >= 0);
  assert(node_add_link_to_subnet(pNodeY1, pSubnetSY3, 16, 100, 1) >= 0);
  assert(node_add_link_to_subnet(pNodeK3, pSubnetSK1, 17, 100, 1) >= 0);
  assert(node_add_link_to_subnet(pNodeK2, pSubnetSK2, 18, 100, 1) >= 0);
  assert(node_add_link_to_subnet(pNodeK2, pSubnetSK3, 19, 100, 1) >= 0);
  
//   LOG_DEBUG("stub-network links attached.\n");
  LOG_DEBUG("ok!\n");
#endif /* OSPF_SUPPORT */
  return 1;
}


/////////////////////////////////////////////////////////////////////
//
// SUBNETS LIST FUNCTIONS
//
/////////////////////////////////////////////////////////////////////

// ----- _subnets_compare -------------------------------------------
/**
 *
 */
static int _subnets_compare(void * pItem1, void * pItem2,
			    unsigned int uEltSize){
  SNetSubnet * pSub1= *((SNetSubnet **) pItem1);
  SNetSubnet * pSub2= *((SNetSubnet **) pItem2);

  return ip_prefix_cmp(&pSub1->sPrefix, &pSub2->sPrefix);
}

// ----- _subnets_destroy -------------------------------------------
static void _subnets_destroy(void * pItem)
{
  subnet_destroy(((SNetSubnet **) pItem));
}

// ----- subnets_create ---------------------------------------------
SNetSubnets * subnets_create()
{
  return ptr_array_create(ARRAY_OPTION_SORTED|
			  ARRAY_OPTION_UNIQUE,
			  _subnets_compare,
			  _subnets_destroy);
}

// ----- subnets_destroy --------------------------------------------
void subnets_destroy(SNetSubnets ** ppSubnets)
{
  ptr_array_destroy((SPtrArray **) ppSubnets);
}

// ----- subnets_add ------------------------------------------------
int subnets_add(SNetSubnets * pSubnets, SNetSubnet * pSubnet)
{
  return ptr_array_add((SPtrArray *) pSubnets, &pSubnet);
}

// ----- subnets_find -----------------------------------------------
SNetSubnet * subnets_find(SNetSubnets * pSubnets, SPrefix sPrefix)
{
  unsigned int uIndex;
  SNetSubnet sWrapSubnet, * pWrapSubnet= &sWrapSubnet;

  sWrapSubnet.sPrefix= sPrefix;
  ip_prefix_mask(&sWrapSubnet.sPrefix);

  if (ptr_array_sorted_find_index(pSubnets, &pWrapSubnet, &uIndex) == 0)
    return (SNetSubnet *) pSubnets->data[uIndex];
  
  return NULL;
}
