// ==================================================================
// @(#)link-list.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 05/08/2003
// @lastdate 05/08/2005
// ==================================================================

#ifndef __NET_LINK_LIST_H__
#define __NET_LINK_LIST_H__

#include <libgds/array.h>
#include <net/net_types.h>

// ----- net_links_create -------------------------------------------
extern SNetLinks * net_links_create();
// ----- net_links_destroy ------------------------------------------
extern void net_links_destroy(SNetLinks ** ppLinks);
// ----- net_links_add ----------------------------------------------
extern int net_links_add(SNetLinks * pLinks, SNetLink * pLink);
// ----- net_links_dump ---------------------------------------------
extern void net_links_dump(FILE * pStream, SNetLinks * pLinks);

// ----- _net_links_link_destroy -------------------------------------
extern void _net_links_link_destroy(void * pItem);

// ----- _net_links_link_compare -------------------------------------
int _net_links_link_compare(void * pItem1, void * pItem2, unsigned int uEltSize); 

#endif /*  __NET_LINK_LIST_H__ */
