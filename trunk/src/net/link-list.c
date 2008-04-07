// ==================================================================
// @(#)link-list.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 05/08/2003
// @lastdate 11/03/2008
// ==================================================================

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <net/iface.h>
#include <net/link.h>
#include <net/link-list.h>

/**
 * IMPORTANT NODE:
 *   The interfaces are identified by their interface address.
 *   However, using only interface addresses as identifier prevents
 *   the following situation where RTR and PTP/PTMP interfaces are
 *   intermixed:
 *
 *         1.0.0.1 <--> 192.168.0.1    (RTR)
 *         1.0.0.1 <--> 192.168.0.1/24 (PTP/PTMP)
 *       
 *   This will not work as both interfaces have the same address.
 *
 *   It is possible to change this behaviour by uncommenting the
 *   following symbol definition. USE THIS WITH CARE as most of the
 *   validation tests make the assumption that interfaces are solely
 *   identified by their address.
 */
//#define __IFACE_ID_ADDR_MASK


// ----- _net_links_link_compare ------------------------------------
/**
 * Links in this list are ordered based on their IDs. A link identifier
 * depends on the interface id (see net_iface_id()).
 */
static int _net_links_link_compare(void * pItem1, void * pItem2,
				   unsigned int uEltSize)
{
  net_iface_t * pIface1= *((net_iface_t **) pItem1);
  net_iface_t * pIface2= *((net_iface_t **) pItem2);

#ifdef __IFACE_ID_ADDR_MASK

  SPrefix sId1= net_iface_id(pIface1);
  SPrefix sId2= net_iface_id(pIface2);

  // Lexicographic order based on (addr, mask)
  // Note that we MUST rely on the unmasked address (tNetwork)
  // We cannot use the ip_prefixes_compare() function !!!
  if (sId1.tNetwork > sId2.tNetwork)
    return 1;
  else if (sId1.tNetwork < sId2.tNetwork)
    return -1;

  if (sId1.uMaskLen > sId2.uMaskLen)
    return 1;
  else if (sId1.uMaskLen < sId2.uMaskLen)
    return -1;

#else

  if (pIface1->tIfaceAddr > pIface2->tIfaceAddr)
    return 1;
  if (pIface1->tIfaceAddr < pIface2->tIfaceAddr)
    return -1;

#endif /* __IFACE_ID_ADDR_MASK */

  return 0;
}

// ----- _net_links_link_destroy -------------------------------------
static void _net_links_link_destroy(void * pItem)
{
  net_iface_t ** ppLink= (net_iface_t **) pItem;
  net_link_destroy(ppLink);
}

// ----- net_links_create -------------------------------------------
net_ifaces_t * net_links_create()
{
  return ptr_array_create(ARRAY_OPTION_SORTED|ARRAY_OPTION_UNIQUE,
			  _net_links_link_compare,
			  _net_links_link_destroy);
}

// ----- net_links_destroy ------------------------------------------
void net_links_destroy(net_ifaces_t ** ppLinks)
{
  ptr_array_destroy(ppLinks);
}

// -----[ net_ifaces_add ]-------------------------------------------
/**
 * Add an interface to the list.
 *
 * Returns
 *   ESUCCESS if the interface could be added
 *   < 0 in case of error (duplicate)
 */
net_error_t net_links_add(net_ifaces_t * pLinks, net_iface_t * pIface)
{
  if (ptr_array_add(pLinks, &pIface) < 0)
    return ENET_IFACE_DUPLICATE;
  return ESUCCESS;
}

// ----- net_links_dump ---------------------------------------------
void net_links_dump(SLogStream * pStream, net_ifaces_t * pIfaces)
{
  unsigned int uIndex;

  for (uIndex= 0; uIndex < net_ifaces_size(pIfaces); uIndex++) {
    net_link_dump(pStream, net_ifaces_at(pIfaces, uIndex));
    log_printf(pStream, "\n");
  }
}

// -----[ net_links_find ]-------------------------------------------
/**
 * Find an interface based on its ID.
 */
net_iface_t * net_links_find(net_ifaces_t * pIfaces, net_iface_id_t tIfaceID)
{
  // Identification in this case is only the destination address
  net_iface_t sWrapIface= {
    .tIfaceAddr= tIfaceID.tNetwork,
#ifdef __IFACE_ID_ADDR_MASK
    .tIfaceMask= tIfaceID.uMaskLen,
#endif /* __IFACE_ID_ADDR_MASK */
  };
  net_iface_t * pWrapIface= &sWrapIface;
  unsigned int uIndex;

  if (ptr_array_sorted_find_index(pIfaces, &pWrapIface, &uIndex) == 0)
    return net_ifaces_at(pIfaces, uIndex);
  return NULL;
}

// ----- net_links_get_enum -----------------------------------------
enum_t * net_links_get_enum(net_ifaces_t * pIfaces)
{
  return _array_get_enum((SArray *) pIfaces);
}

// -----[ net_links_find_iface ]-------------------------------------
net_iface_t * net_links_find_iface(net_ifaces_t * pIfaces,
				   net_addr_t tIfaceAddr)
{
  net_iface_t * pIface;
  unsigned int uIndex;
  SPrefix sPrefix;

  for (uIndex= 0; uIndex < net_ifaces_size(pIfaces); uIndex++) {
    pIface= net_ifaces_at(pIfaces, uIndex);
    sPrefix= net_iface_id(pIface);
    if (tIfaceAddr == sPrefix.tNetwork)
      return pIface;
  }
  return NULL;
}

// ----- net_links_get_smaller_iface --------------------------------
//
// Returns smaller iface addr in links list.
// Used to obtain correct OSPF-ID of a node.
/*
net_addr_t net_links_get_smaller_iface(net_iface_ts * pLinks)
{
   net_iface_t * pLink = NULL;
   int iIndex;
   for (iIndex = 0; iIndex < ptr_array_length(pLinks); iIndex++){
     ptr_array_get_at(pLinks, ptr_array_length(pLinks)-1, &pLink);
     if (pLink->tType == NET_IFACE_RTR)
       continue;
     
     return pLink->tIfaceAddr;
   }
	     
   if (pLink != NULL)
     return pLink->pSrcNode->tAddr;
   
   return 0;
}
*/
