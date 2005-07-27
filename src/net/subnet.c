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


/////////////////////////////////////////////////////////////////////////////////
// SNetSubnet Methods
/////////////////////////////////////////////////////////////////////////////////

// ----- subnet_create ----------------------------------------------------------
SNetSubnet * subnet_create(net_addr_t tNetwork, uint8_t uMaskLen, uint8_t uType)
{
  SNetSubnet * pSubnet= (SNetSubnet *) MALLOC(sizeof(SNetSubnet));
  (pSubnet->sPrefix).tNetwork= tNetwork;
  (pSubnet->sPrefix).uMaskLen= uMaskLen;
  pSubnet->uOSPFArea = OSPF_NO_AREA;
  pSubnet->uType = uType;
  
//   pSubnet->aNodes = ptr_array_create(ARRAY_OPTION_SORTED|ARRAY_OPTION_UNIQUE,
// 				  node_compare,
// 				  NULL);
  pSubnet->pLinks = ptr_array_create(ARRAY_OPTION_SORTED|ARRAY_OPTION_UNIQUE,
				  node_links_compare,
				  node_links_destroy);
  return pSubnet; 
}

// ----- subnet_destroy ----------------------------------------------------------
void subnet_destroy(SNetSubnet ** ppSubnet)
{
  if (*ppSubnet != NULL) {
//     fprintf(stdout, "destroing subnet: ");
//     ip_prefix_dump(stdout, *(subnet_get_prefix(*ppSubnet)));
//     fprintf(stdout, "\n");
    ptr_array_destroy(&(*ppSubnet)->pLinks);
    FREE(*ppSubnet);
    *ppSubnet= NULL;
  }
}

// --------- subnet_dump ---------------------------------------
void subnet_dump(FILE * pStream, SNetSubnet * pSubnet)
{
  char pcAddr[16];
  SNetLink * pLink = NULL;
  char pcPrefix[30];
  int iIndex;
  
  ip_prefix_to_string(pcPrefix, &(pSubnet->sPrefix));
  fprintf(pStream, "SUBNET PREFIX <%s>\t", pcPrefix);
  fprintf(pStream, "OSPF AREA  <%d>\t", pSubnet->uOSPFArea);
  switch (pSubnet->uType) {
    case NET_SUBNET_TYPE_TRANSIT : 
           fprintf(pStream, "TRANSIT\t");
         break;
    case NET_SUBNET_TYPE_STUB : 
           fprintf(pStream, "STUB\t");
         break;
    default : fprintf(pStream, "???\t");
  }
  fprintf(pStream, "LINKED TO: \t");
   
  for (iIndex = 0; iIndex < ptr_array_length(pSubnet->pLinks); iIndex++) {
    ptr_array_get_at(pSubnet->pLinks, iIndex, &pLink);
    assert(pLink != NULL);
    ip_address_to_string(pcAddr, link_get_address(pLink));
    fprintf(pStream, "%s\n", pcAddr);
    fprintf(pStream, "---\t\t\t\t\t\t");
  }
  fprintf(pStream,"\n");
}

// ----- subnet_get_links -------------------------------------------------- 
links_list_t * subnet_get_links(SNetSubnet * pSubnet) 
{
//   SPtrArray * aLinks;
//   int iIndexN, iIndexL;
//   SNetNode * pNode;
//   SNetLink * pLink = NULL;
  
  return pSubnet->pLinks;
 /* aLinks = ptr_array_create(0,  node_links_compare,  node_links_destroy);
       
       assert(pSubnet->pLinks != NULL);
       assert(pSubnet->pPrefix != NULL);
       
       for (iIndexN= 0; iIndexN < ptr_array_length(pSubnet->pLinks); iIndexN++) {
         ptr_array_get_at(pSubnet->pLinks, iIndexN, &pNode);
	 assert(pNode != NULL);
	 

	 for (iIndexL = 0; iIndexL < ptr_array_length(pNode->pLinks); iIndexL++) {
	   ptr_array_get_at(pNode->pLinks, iIndexL, &pLink);
	   assert(pLink != NULL);
	   if (link_get_address(pLink) == pSubnet->pPrefix->tNetwork){
	     SNetLink * pNewLink        = create_link_toRouter(pNode);
	     pNewLink->uDestinationType = NET_LINK_TYPE_ROUTER;
	     pNewLink->uFlags           = pLink->uFlags;
	     pNewLink->uIGPweight       = 0;
	     assert(ptr_array_add(aLinks, &pNewLink)>=0);
	   }
	 }
	 
       }
  return aLinks;*/
}

// ----- subnet_link_to_node ----------------------------------------
// ASSUMPTION: the link towards node does not exist yet
//WARNING: this should be invoke only by node_add_link_toSubnet
int subnet_link_toNode(SNetSubnet * pSubnet, SNetNode * pNode){
//   return ptr_array_add(pSubnet->pLinks, &pNode);
  SNetLink * pNewLink = (SNetLink *) MALLOC(sizeof(SNetLink));
  pNewLink->uDestinationType = NET_LINK_TYPE_ROUTER;
  (pNewLink->UDestId).tAddr = pNode->tAddr;
  
  pNewLink->uIGPweight= 0;
  pNewLink->uFlags = NET_LINK_FLAG_UP;
  pNewLink->pContext= NULL;
  pNewLink->fForward= NULL;
  
  return ptr_array_add(pSubnet->pLinks, &pNewLink);
}

// ----- subnet_getAddr --------------------------------------------------
SPrefix * subnet_get_prefix(SNetSubnet * pSubnet)
{
  return &(pSubnet->sPrefix);
}

// ----- subnet_is_transit -------------------------------------------------
int subnet_is_transit(SNetSubnet * pSubnet) {
  return (pSubnet->uType == NET_SUBNET_TYPE_TRANSIT);
}

// ----- subnet_is_stub -------------------------------------------------
int subnet_is_stub(SNetSubnet * pSubnet) {
  return (pSubnet->uType == NET_SUBNET_TYPE_STUB);
}

// ----- subnets_compare -----------------------------------------
int subnets_compare(void * pItem1, void * pItem2,
		       unsigned int uEltSize){
  SNetSubnet * pSub1= *((SNetSubnet **) pItem1);
  SNetSubnet * pSub2= *((SNetSubnet **) pItem2);
  
  SPrefix * pPfx1 = &(pSub1->sPrefix);
  SPrefix * pPfx2 = &(pSub2->sPrefix);
  
  return ip_prefixes_compare(&pPfx1, &pPfx2, 0);   
}

int subnet_test(){
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
  //subnet_dump(stdout, pSubTx1);
  
  assert(node_add_link_toSubnet(pNodeA, pSubTx, 100, 1) >= 0);
  assert(node_add_link_toSubnet(pNodeB, pSubTx, 100, 1) >= 0);
  assert(node_add_link_toSubnet(pNodeC, pSubTx, 100, 1) >= 0);
  assert(node_add_link(pNodeA, pNodeC, 100, 1) >= 0);
  
  
  SNetLink * pLinkAC = node_find_link_to_router(pNodeA, tAddrC);
  assert(link_get_address(pLinkAC) == (tAddrC));
  SNetLink * pLinkAB = node_find_link_to_router(pNodeA, tAddrB);
  assert(pLinkAB == NULL);
  
  SNetLink * pLinkAS = node_find_link_to_subnet(pNodeA, pSubTx);
  assert(link_get_address(pLinkAS) == ((pSubTx->sPrefix).tNetwork));
  SNetLink * pLinkAS1 = node_find_link_to_subnet(pNodeA, pSubTx1);
  assert(pLinkAS1 == NULL);
  
  subnet_destroy(&pSubTx);
  subnet_destroy(&pSubTx1);
  
  node_destroy(&pNodeA);
  node_destroy(&pNodeB);
  node_destroy(&pNodeC);
  
  LOG_DEBUG("ok!\n");
  LOG_DEBUG("subnet_test(): TEST on a bigger network... \n");
  /* other test on subnet methods... */
  const int startAddr = 1024;
  
  SNetwork * pNetwork= network_create();
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

  assert(!network_add_node(pNetwork, pNodeB1));
  assert(!network_add_node(pNetwork, pNodeB2));
  assert(!network_add_node(pNetwork, pNodeB3));
  assert(!network_add_node(pNetwork, pNodeB4));
  assert(!network_add_node(pNetwork, pNodeX1));
  assert(!network_add_node(pNetwork, pNodeX2));
  assert(!network_add_node(pNetwork, pNodeX3));
  assert(!network_add_node(pNetwork, pNodeY1));
  assert(!network_add_node(pNetwork, pNodeY2));
  assert(!network_add_node(pNetwork, pNodeK1));
  assert(!network_add_node(pNetwork, pNodeK2));
  assert(!network_add_node(pNetwork, pNodeK3));
  
  assert(network_add_subnet(pNetwork, pSubnetTB1) >= 0);
  assert(network_add_subnet(pNetwork, pSubnetTX1) >= 0);
  assert(network_add_subnet(pNetwork, pSubnetTY1) >= 0);
  assert(network_add_subnet(pNetwork, pSubnetTK1) >= 0);
  
  assert(network_add_subnet(pNetwork, pSubnetSB1) >= 0);
  assert(network_add_subnet(pNetwork, pSubnetSB2) >= 0);
  assert(network_add_subnet(pNetwork, pSubnetSX1) >= 0);
  assert(network_add_subnet(pNetwork, pSubnetSX2) >= 0);
  assert(network_add_subnet(pNetwork, pSubnetSX3) >= 0);
  assert(network_add_subnet(pNetwork, pSubnetSY1) >= 0);
  assert(network_add_subnet(pNetwork, pSubnetSY2) >= 0);
  assert(network_add_subnet(pNetwork, pSubnetSY3) >= 0);
  assert(network_add_subnet(pNetwork, pSubnetSK1) >= 0);
  assert(network_add_subnet(pNetwork, pSubnetSK2) >= 0);
  assert(network_add_subnet(pNetwork, pSubnetSK3) >= 0);
  
  
//   LOG_DEBUG("nodes attached.\n");

  assert(node_add_link(pNodeB1, pNodeB2, 100, 1) >= 0);
  assert(node_add_link(pNodeB2, pNodeB3, 100, 1) >= 0);
  assert(node_add_link(pNodeB3, pNodeB4, 100, 1) >= 0);
  assert(node_add_link(pNodeB4, pNodeB1, 100, 1) >= 0);
  
  assert(node_add_link(pNodeB2, pNodeX2, 100, 1) >= 0);
  assert(node_add_link(pNodeB3, pNodeX2, 100, 1) >= 0);
  assert(node_add_link(pNodeX2, pNodeX1, 100, 1) >= 0);
  assert(node_add_link(pNodeX2, pNodeX3, 100, 1) >= 0);

  assert(node_add_link(pNodeB3, pNodeY1, 100, 1) >= 0);
  assert(node_add_link(pNodeY1, pNodeY2, 100, 1) >= 0);
  
  assert(node_add_link(pNodeB4, pNodeK1, 100, 1) >= 0);
  assert(node_add_link(pNodeB4, pNodeK2, 100, 1) >= 0);
  assert(node_add_link(pNodeB1, pNodeK3, 100, 1) >= 0);
  assert(node_add_link(pNodeK3, pNodeK2, 100, 1) >= 0);
  
//   LOG_DEBUG("point-to-point links attached.\n");
  
  assert(node_add_link_toSubnet(pNodeB1, pSubnetTB1, 100, 1) >= 0);
  assert(node_add_link_toSubnet(pNodeB3, pSubnetTB1, 100, 1) >= 0);
  assert(node_add_link_toSubnet(pNodeX1, pSubnetTX1, 100, 1) >= 0);
  assert(node_add_link_toSubnet(pNodeX3, pSubnetTX1, 100, 1) >= 0);
  assert(node_add_link_toSubnet(pNodeB3, pSubnetTY1, 100, 1) >= 0);
  assert(node_add_link_toSubnet(pNodeY2, pSubnetTY1, 100, 1) >= 0);
  assert(node_add_link_toSubnet(pNodeK2, pSubnetTK1, 100, 1) >= 0);
  assert(node_add_link_toSubnet(pNodeK1, pSubnetTK1, 100, 1) >= 0);
  
//   LOG_DEBUG("transit-network links attached.\n");
  
  assert(node_add_link_toSubnet(pNodeB2, pSubnetSB1, 100, 1) >= 0);
  assert(node_add_link_toSubnet(pNodeB1, pSubnetSB2, 100, 1) >= 0);
  assert(node_add_link_toSubnet(pNodeB2, pSubnetSX1, 100, 1) >= 0);
  assert(node_add_link_toSubnet(pNodeX2, pSubnetSX2, 100, 1) >= 0);
  assert(node_add_link_toSubnet(pNodeX3, pSubnetSX3, 100, 1) >= 0);
  assert(node_add_link_toSubnet(pNodeB3, pSubnetSY1, 100, 1) >= 0);
  assert(node_add_link_toSubnet(pNodeY1, pSubnetSY2, 100, 1) >= 0);
  assert(node_add_link_toSubnet(pNodeY1, pSubnetSY3, 100, 1) >= 0);
  assert(node_add_link_toSubnet(pNodeK3, pSubnetSK1, 100, 1) >= 0);
  assert(node_add_link_toSubnet(pNodeK2, pSubnetSK2, 100, 1) >= 0);
  assert(node_add_link_toSubnet(pNodeK2, pSubnetSK3, 100, 1) >= 0);
  
//   LOG_DEBUG("stub-network links attached.\n");
  LOG_DEBUG("ok!\n");
  
  network_destroy(&pNetwork);

  return 1;
}

