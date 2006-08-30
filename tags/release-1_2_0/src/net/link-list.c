// ==================================================================
// @(#)link-list.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 05/08/2003
// @lastdate 03/03/2006
// ==================================================================

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <net/link.h>
#include <net/link-list.h>

// ----- _net_links_link_compare ------------------------------------
int _net_links_link_compare(void * pItem1, void * pItem2,
			    unsigned int uEltSize)
{
  SNetLink * pLink1= *((SNetLink **) pItem1);
  SNetLink * pLink2= *((SNetLink **) pItem2);

  if (link_get_id(pLink1) < link_get_id(pLink2)) {
    return 1;
  } else if (link_get_id(pLink1) > link_get_id(pLink2)) {
    return -1;
  }

  return 0;
}

// ----- _net_links_link_destroy -------------------------------------
void _net_links_link_destroy(void * pItem)
{
  SNetLink ** ppLink= (SNetLink **) pItem;
  link_destroy(ppLink);
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
    link_dump(pStream, (SNetLink *) pLinks->data[uIndex]);
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
     if (pLink->uDestinationType == NET_LINK_TYPE_ROUTER)
       continue;
     
     return pLink->tIfaceAddr;
   }
	     
   if (pLink != NULL)
     return pLink->pSrcNode->tAddr;
   
   return 0;
}

	
