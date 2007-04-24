// ==================================================================
// @(#)link-list.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 05/08/2003
// @lastdate 13/04/2007
// ==================================================================

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <net/link.h>
#include <net/link-list.h>

// ----- _net_links_link_compare ------------------------------------
/**
 * Links in this list are ordered based on their IDs. A link identifier
 * depends on the link type:
 *   ptp: dst address + mask length (32)
 *   mtp: local interface address + subnet's mask length
 */
static int _net_links_link_compare(void * pItem1, void * pItem2,
				   unsigned int uEltSize)
{
  SNetLink * pLink1= *((SNetLink **) pItem1);
  SNetLink * pLink2= *((SNetLink **) pItem2);
  SPrefix sId1= net_link_get_id(pLink1);
  SPrefix sId2= net_link_get_id(pLink2);

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

  return 0;
}

// ----- _net_links_link_destroy -------------------------------------
static void _net_links_link_destroy(void * pItem)
{
  SNetLink ** ppLink= (SNetLink **) pItem;
  net_link_destroy(ppLink);
}

// ----- net_links_create -------------------------------------------
SNetLinks * net_links_create()
{
  return ptr_array_create(ARRAY_OPTION_SORTED|ARRAY_OPTION_UNIQUE,
			  _net_links_link_compare,
			  _net_links_link_destroy);
}

// ----- net_links_destroy ------------------------------------------
void net_links_destroy(SNetLinks ** ppLinks)
{
  ptr_array_destroy(ppLinks);
}

// ----- net_links_add ----------------------------------------------
int net_links_add(SNetLinks * pLinks, SNetLink * pLink)
{
  return ptr_array_add(pLinks, &pLink);
}

// ----- net_links_dump ---------------------------------------------
void net_links_dump(SLogStream * pStream, SNetLinks * pLinks)
{
  unsigned int uIndex;

  for (uIndex= 0; uIndex < ptr_array_length(pLinks); uIndex++) {
    net_link_dump(pStream, (SNetLink *) pLinks->data[uIndex]);
    log_printf(pStream, "\n");
  }
}

// ----- net_links_get_smaller_iface --------------------------------
//
// Returns smaller iface addr in links list.
// Used to obtain correct OSPF-ID of a node.
net_addr_t net_links_get_smaller_iface(SNetLinks * pLinks)
{
   SNetLink * pLink = NULL;
   int iIndex;
   for (iIndex = 0; iIndex < ptr_array_length(pLinks); iIndex++){
     ptr_array_get_at(pLinks, ptr_array_length(pLinks)-1, &pLink);
     if (pLink->uType == NET_LINK_TYPE_ROUTER)
       continue;
     
     return pLink->tIfaceAddr;
   }
	     
   if (pLink != NULL)
     return pLink->pSrcNode->tAddr;
   
   return 0;
}

// ----- net_links_find_ptp -----------------------------------------
/**
 * Find the ptp link to router identified by destination address.
 */
SNetLink * net_links_find_ptp(SNetLinks * pLinks, net_addr_t tDstAddr)
{
  unsigned int uIndex;
  SNetLink sWrapLink, * pWrapLink= &sWrapLink;

  // Identification in this case is only the destination address
  sWrapLink.uType= NET_LINK_TYPE_ROUTER;
  sWrapLink.tDest.tAddr= tDstAddr;

  if (ptr_array_sorted_find_index(pLinks, &pWrapLink, &uIndex) == 0)
    return (SNetLink *) pLinks->data[uIndex];

  return NULL;
}

// ----- net_links_find_mtp -----------------------------------------
/**
 * Find the mtp link identified by local interface address.
 */
SNetLink * net_links_find_mtp(SNetLinks * pLinks, net_addr_t tIfaceAddr,
			      net_mask_t tIfaceMask)
{
  unsigned int uIndex;
  SNetLink sWrapLink, * pWrapLink= &sWrapLink;
  
  // Identification in this case is the local interface address +
  // the subnet's mask length
  sWrapLink.uType= NET_LINK_TYPE_TRANSIT;
  sWrapLink.tIfaceAddr= tIfaceAddr;
  sWrapLink.tIfaceMask= tIfaceMask;

  if (ptr_array_sorted_find_index(pLinks, &pWrapLink, &uIndex) == 0)
    return (SNetLink *) pLinks->data[uIndex];

  return NULL;
}

// ----- net_links_get_enum -----------------------------------------
SEnumerator * net_links_get_enum(SNetLinks * pLinks)
{
  return _array_get_enum((SArray *) pLinks);
}

// -----[ net_links_find_iface ]-------------------------------------
SNetLink * net_links_find_iface(SNetLinks * pLinks, net_addr_t tIfaceAddr)
{
  SNetLink * pLink;
  unsigned int uIndex;
  SPrefix sPrefix;

  for (uIndex= 0; uIndex < ptr_array_length(pLinks); uIndex++) {
    pLink= (SNetLink *) pLinks->data[uIndex];

    sPrefix= net_link_get_id(pLink);

    if (tIfaceAddr == sPrefix.tNetwork)
      return pLink;
  }
  return NULL;
}
