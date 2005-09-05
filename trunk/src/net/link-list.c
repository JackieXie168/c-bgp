// ==================================================================
// @(#)link-list.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 05/08/2003
// @lastdate 01/09/2005
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

  if (link_get_iface(pLink1) < link_get_iface(pLink2)) {
    return 1;
  } else if (link_get_iface(pLink1) > link_get_iface(pLink2)) {
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
void net_links_dump(FILE * pStream, SNetLinks * pLinks)
{
  unsigned int uIndex;

  for (uIndex= 0; uIndex < ptr_array_length(pLinks); uIndex++) {
    link_dump(pStream, (SNetLink *) pLinks->data[uIndex]);
    fprintf(pStream, "\n");
  }
}
