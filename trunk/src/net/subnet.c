// ==================================================================
// @(#)subnet.c
//
// @author Stefano Iasi (stefanoia@tin.it)
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 14/06/2005
// @lastdate 11/03/2008
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
// net_subnet_t Methods
//
/////////////////////////////////////////////////////////////////////

// ----- _net_subnet_link_compare -----------------------------------
static int _net_subnet_link_compare(void * pItem1, void * pItem2,
				    unsigned int uEltSize)
{
  net_iface_t * pLink1= *((net_iface_t **) pItem1);
  net_iface_t * pLink2= *((net_iface_t **) pItem2);

  if (pLink1->tIfaceAddr > pLink2->tIfaceAddr)
    return 1;
  if (pLink1->tIfaceAddr < pLink2->tIfaceAddr)
    return -1;
  
  return 0;
}

// ----- subnet_create ----------------------------------------------
net_subnet_t * subnet_create(net_addr_t tNetwork, uint8_t uMaskLen,
			   uint8_t uType)
{
  net_subnet_t * subnet= (net_subnet_t *) MALLOC(sizeof(net_subnet_t));
  assert(uMaskLen < 32); // subnets with a mask of /32 are not allowed
  subnet->sPrefix.tNetwork= tNetwork;
  subnet->sPrefix.uMaskLen= uMaskLen;
  ip_prefix_mask(&subnet->sPrefix);
  subnet->uType = uType;
  
#ifdef OSPF_SUPPORT
  subnet->uOSPFArea = OSPF_NO_AREA;
#endif

  // Create an array of references to links
  subnet->ifaces= ptr_array_create(ARRAY_OPTION_SORTED |
				    ARRAY_OPTION_UNIQUE,
				    _net_subnet_link_compare,
				    NULL);
  return subnet; 
}

// ----- subnet_destroy ---------------------------------------------
void subnet_destroy(net_subnet_t ** subnet_ref)
{
  if (*subnet_ref != NULL) {
    net_links_destroy(&(*subnet_ref)->ifaces);
    FREE(*subnet_ref);
    *subnet_ref= NULL;
  }
}

// ----- subnet_dump ------------------------------------------------
void subnet_dump(SLogStream * stream, net_subnet_t * subnet)
{
  net_iface_t * pLink = NULL;
  unsigned int index;
  
  log_printf(stream, "SUBNET PREFIX <");
  ip_prefix_dump(stream, subnet->sPrefix);
#ifdef OSPF_SUPPORT
  log_printf(stream, ">\tOSPF AREA  <%u>\t",
	     (unsigned int) subnet->uOSPFArea);
#endif
  switch (subnet->uType) {
  case NET_SUBNET_TYPE_TRANSIT : 
    log_printf(stream, "TRANSIT\t");
    break;
  case NET_SUBNET_TYPE_STUB : 
    log_printf(stream, "STUB\t");
    break;
  default:
    log_printf(stream, "???\t");
  }
  log_printf(stream, "LINKED TO: \t");
  //dump links 
  for (index = 0; index < ptr_array_length(subnet->ifaces); index++) {
    ptr_array_get_at(subnet->ifaces, index, &pLink);
    assert(pLink != NULL);
    ip_address_dump(stream, pLink->tIfaceAddr);
    log_printf(stream, "\n---\t\t\t\t\t\t");
  }
  log_printf(stream,"\n");
}

// -----[ subnet_add_link ]------------------------------------------
int subnet_add_link(net_subnet_t * subnet, net_iface_t * pLink)
{
  if (ptr_array_add(subnet->ifaces, &pLink) < 0) {
    return ENET_LINK_DUPLICATE;
  }

  return ESUCCESS;
}

// ----- subnet_getAddr ---------------------------------------------
SPrefix * subnet_get_prefix(net_subnet_t * subnet) {
  return &(subnet->sPrefix);
}

// ----- subnet_is_transit ------------------------------------------
int subnet_is_transit(net_subnet_t * subnet) {
  return (subnet->uType == NET_SUBNET_TYPE_TRANSIT);
}

// ----- subnet_is_stub ---------------------------------------------
int subnet_is_stub(net_subnet_t * subnet) {
  return (subnet->uType == NET_SUBNET_TYPE_STUB);
}

// ----- subnet_find_link -------------------------------------------
/**
 * Find a link based on its interface address.
 */
net_iface_t * net_subnet_find_link(net_subnet_t * subnet, net_addr_t dst_addr)
{
  net_iface_t sWrapLink= { .tIfaceAddr= dst_addr };
  net_iface_t * pLink= NULL;
  unsigned int index;
  net_iface_t * pWrapLink= &sWrapLink;

  if (ptr_array_sorted_find_index(subnet->ifaces, &pWrapLink, &index) == 0)
    pLink= net_ifaces_at(subnet->ifaces, index);
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
  net_node_t * pNodeA= node_create(tAddrA);
  net_node_t * pNodeB= node_create(tAddrB);
  net_node_t * pNodeC= node_create(tAddrC);
  
  assert(!ip_string_to_prefix("192.168.0.0/24", &pcEndPtr, &sSubnetPfxTx));
  assert(!ip_string_to_prefix("192.120.0.0/24", &pcEndPtr, &sSubnetPfxTx1));
  
  //ip_prefix_dump(stdout, sSubnetPfxTx);
  //ip_prefix_dump(stdout, sSubnetPfxTx1);
  
  net_subnet_t * pSubTx = subnet_create(sSubnetPfxTx.tNetwork,
				      sSubnetPfxTx.uMaskLen, NET_SUBNET_TYPE_TRANSIT);
  net_subnet_t * pSubTx1 = subnet_create(sSubnetPfxTx1.tNetwork,
				       sSubnetPfxTx.uMaskLen,
				       NET_SUBNET_TYPE_TRANSIT);
  //subnet_dump(stdout, pSubTx);
  //subnet_dump(stdout, pSubTx1);a
  net_addr_t ipIfaceA = IPV4_TO_INT(192,168,0,1);
  net_addr_t ipIfaceB = IPV4_TO_INT(192,168,0,2); 
  net_addr_t ipIfaceC = IPV4_TO_INT(192,168,0,3);
  assert(node_add_link_to_subnet(pNodeA, pSubTx, ipIfaceA, 100, 1) == ESUCCESS);
  assert(node_add_link_to_subnet(pNodeB, pSubTx, ipIfaceB, 100, 1) == ESUCCESS);
  assert(node_add_link_to_subnet(pNodeC, pSubTx, ipIfaceC, 100, 1) == ESUCCESS);
  assert(node_add_link_to_router(pNodeA, pNodeC, 100, 1) == ESUCCESS);
  
  
  net_iface_t * pLinkAC = node_find_link_to_router(pNodeA, tAddrC);
  assert(link_get_address(pLinkAC) == (tAddrC));
  
  net_iface_t * pLinkAB = node_find_link_to_router(pNodeA, tAddrB);
  assert(pLinkAB == NULL);
  
  net_iface_t * pLinkAS = node_find_link_to_subnet(pNodeA, pSubTx, ipIfaceA);
  assert(pLinkAS != NULL);
  assert(link_get_address(pLinkAS) == ((pSubTx->sPrefix).tNetwork));

  net_iface_t * pLinkAS1 = node_find_link_to_subnet(pNodeA, pSubTx1, ipIfaceB);
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
  net_node_t * pNodeB1= node_create(startAddr);
  net_node_t * pNodeB2= node_create(startAddr + 1);
  net_node_t * pNodeB3= node_create(startAddr + 2);
  net_node_t * pNodeB4= node_create(startAddr + 3);
  net_node_t * pNodeX1= node_create(startAddr + 4);
  net_node_t * pNodeX2= node_create(startAddr + 5);
  net_node_t * pNodeX3= node_create(startAddr + 6);
  net_node_t * pNodeY1= node_create(startAddr + 7);
  net_node_t * pNodeY2= node_create(startAddr + 8);
  net_node_t * pNodeK1= node_create(startAddr + 9);
  net_node_t * pNodeK2= node_create(startAddr + 10);
  net_node_t * pNodeK3= node_create(startAddr + 11);
  
  net_subnet_t * subnetTB1= subnet_create(4, 30, NET_SUBNET_TYPE_TRANSIT);
  net_subnet_t * subnetTX1= subnet_create(8, 30, NET_SUBNET_TYPE_TRANSIT);
  net_subnet_t * subnetTY1= subnet_create(12, 30, NET_SUBNET_TYPE_TRANSIT);
  net_subnet_t * subnetTK1= subnet_create(16, 30, NET_SUBNET_TYPE_TRANSIT);
  
  net_subnet_t * subnetSB1= subnet_create(20, 30, NET_SUBNET_TYPE_STUB);
  net_subnet_t * subnetSB2= subnet_create(24, 30, NET_SUBNET_TYPE_STUB);
  net_subnet_t * subnetSX1= subnet_create(28, 30, NET_SUBNET_TYPE_STUB);
  net_subnet_t * subnetSX2= subnet_create(32, 30, NET_SUBNET_TYPE_STUB);
  net_subnet_t * subnetSX3= subnet_create(36, 30, NET_SUBNET_TYPE_STUB);
  net_subnet_t * subnetSY1= subnet_create(40, 30, NET_SUBNET_TYPE_STUB);
  net_subnet_t * subnetSY2= subnet_create(44, 30, NET_SUBNET_TYPE_STUB);
  net_subnet_t * subnetSY3= subnet_create(48, 30, NET_SUBNET_TYPE_STUB);
  net_subnet_t * subnetSK1= subnet_create(52, 30, NET_SUBNET_TYPE_STUB);
  net_subnet_t * subnetSK2= subnet_create(56, 30, NET_SUBNET_TYPE_STUB);
  net_subnet_t * subnetSK3= subnet_create(60, 30, NET_SUBNET_TYPE_STUB);

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
  
  assert(network_add_subnet(subnetTB1) >= 0);
  assert(network_add_subnet(subnetTX1) >= 0);
  assert(network_add_subnet(subnetTY1) >= 0);
  assert(network_add_subnet(subnetTK1) >= 0);
  
  assert(network_add_subnet(subnetSB1) >= 0);
  assert(network_add_subnet(subnetSB2) >= 0);
  assert(network_add_subnet(subnetSX1) >= 0);
  assert(network_add_subnet(subnetSX2) >= 0);
  assert(network_add_subnet(subnetSX3) >= 0);
  assert(network_add_subnet(subnetSY1) >= 0);
  assert(network_add_subnet(subnetSY2) >= 0);
  assert(network_add_subnet(subnetSY3) >= 0);
  assert(network_add_subnet(subnetSK1) >= 0);
  assert(network_add_subnet(subnetSK2) >= 0);
  assert(network_add_subnet(subnetSK3) >= 0);
  
  
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
  
  assert(node_add_link_to_subnet(pNodeB1, subnetTB1, 1, 100, 1) >= 0);
  assert(node_add_link_to_subnet(pNodeB3, subnetTB1, 2, 100, 1) >= 0);
  assert(node_add_link_to_subnet(pNodeX1, subnetTX1, 3, 100, 1) >= 0);
  assert(node_add_link_to_subnet(pNodeX3, subnetTX1, 4, 100, 1) >= 0);
  assert(node_add_link_to_subnet(pNodeB3, subnetTY1, 5, 100, 1) >= 0);
  assert(node_add_link_to_subnet(pNodeY2, subnetTY1, 6, 100, 1) >= 0);
  assert(node_add_link_to_subnet(pNodeK2, subnetTK1, 7, 100, 1) >= 0);
  assert(node_add_link_to_subnet(pNodeK1, subnetTK1, 8, 100, 1) >= 0);
 
//   LOG_DEBUG("transit-network links attached.\n");
  
  assert(node_add_link_to_subnet(pNodeB2, subnetSB1, 9, 100, 1) >= 0);
  assert(node_add_link_to_subnet(pNodeB1, subnetSB2, 10, 100, 1) >= 0);
  assert(node_add_link_to_subnet(pNodeB2, subnetSX1, 11, 100, 1) >= 0);
  assert(node_add_link_to_subnet(pNodeX2, subnetSX2, 12, 100, 1) >= 0);
  assert(node_add_link_to_subnet(pNodeX3, subnetSX3, 13, 100, 1) >= 0);
  assert(node_add_link_to_subnet(pNodeB3, subnetSY1, 14, 100, 1) >= 0);
  assert(node_add_link_to_subnet(pNodeY1, subnetSY2, 15, 100, 1) >= 0);
  assert(node_add_link_to_subnet(pNodeY1, subnetSY3, 16, 100, 1) >= 0);
  assert(node_add_link_to_subnet(pNodeK3, subnetSK1, 17, 100, 1) >= 0);
  assert(node_add_link_to_subnet(pNodeK2, subnetSK2, 18, 100, 1) >= 0);
  assert(node_add_link_to_subnet(pNodeK2, subnetSK3, 19, 100, 1) >= 0);
  
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
  net_subnet_t * pSub1= *((net_subnet_t **) pItem1);
  net_subnet_t * pSub2= *((net_subnet_t **) pItem2);

  return ip_prefix_cmp(&pSub1->sPrefix, &pSub2->sPrefix);
}

// ----- _subnets_destroy -------------------------------------------
static void _subnets_destroy(void * pItem)
{
  subnet_destroy(((net_subnet_t **) pItem));
}

// ----- subnets_create ---------------------------------------------
net_subnets_t * subnets_create()
{
  return ptr_array_create(ARRAY_OPTION_SORTED|
			  ARRAY_OPTION_UNIQUE,
			  _subnets_compare,
			  _subnets_destroy);
}

// ----- subnets_destroy --------------------------------------------
void subnets_destroy(net_subnets_t ** psubnets)
{
  ptr_array_destroy((SPtrArray **) psubnets);
}

// ----- subnets_add ------------------------------------------------
int subnets_add(net_subnets_t * subnets, net_subnet_t * subnet)
{
  return ptr_array_add((SPtrArray *) subnets, &subnet);
}

// ----- subnets_find -----------------------------------------------
net_subnet_t * subnets_find(net_subnets_t * subnets, SPrefix sPrefix)
{
  unsigned int index;
  net_subnet_t sWrasubnet, * pWrasubnet= &sWrasubnet;

  sWrasubnet.sPrefix= sPrefix;
  ip_prefix_mask(&sWrasubnet.sPrefix);

  if (ptr_array_sorted_find_index(subnets, &pWrasubnet, &index) == 0)
    return (net_subnet_t *) subnets->data[index];
  
  return NULL;
}
