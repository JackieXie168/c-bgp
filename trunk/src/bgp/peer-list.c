// ==================================================================
// @(#)peer-list.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 10/03/2008
// @lastdate 10/03/2008
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <net/error.h>

#include <bgp/peer.h>
#include <bgp/peer-list.h>

// -----[ _bgp_peers_compare ]---------------------------------------
static int _bgp_peers_compare(void * pItem1, void * pItem2,
			      unsigned int uEltSize)
{
  bgp_peer_t * pPeer1= *((bgp_peer_t **) pItem1);
  bgp_peer_t * pPeer2= *((bgp_peer_t **) pItem2);

  if (pPeer1->tAddr < pPeer2->tAddr)
    return -1;
  else if (pPeer1->tAddr > pPeer2->tAddr)
    return 1;
  else
    return 0;
}

// -----[ _bgp_peers_destroy ]---------------------------------------
static void _bgp_peers_destroy(void * pItem)
{
  bgp_peer_t * pPeer= *((bgp_peer_t **) pItem);
  bgp_peer_destroy(&pPeer);
}

// -----[ bgp_peers_create ]-----------------------------------------
bgp_peers_t * bgp_peers_create()
{
  return ptr_array_create(ARRAY_OPTION_SORTED |
			  ARRAY_OPTION_UNIQUE,
			  _bgp_peers_compare,
			  _bgp_peers_destroy);
}

// -----[ bgp_peers_destroy ]----------------------------------------
void bgp_peers_destroy(bgp_peers_t ** ppPeers)
{
  ptr_array_destroy(ppPeers);
}

// -----[ bgp_peers_add ]--------------------------------------------
net_error_t bgp_peers_add(bgp_peers_t * pPeers, bgp_peer_t * pPeer)
{
  if (ptr_array_add(pPeers, &pPeer) < 0)
    return EBGP_PEER_DUPLICATE;
  return ESUCCESS;
}

// -----[ bgp_peers_find ]-------------------------------------------
bgp_peer_t * bgp_peers_find(bgp_peers_t * pPeers, net_addr_t tAddr)
{
  unsigned int uIndex;
  bgp_peer_t tDummyPeer= { .tAddr= tAddr }, * pDummyPeer= &tDummyPeer;
  bgp_peer_t * pPeer= NULL;

  if (ptr_array_sorted_find_index(pPeers, &pDummyPeer, &uIndex) != -1)
    pPeer= (bgp_peer_t *) pPeers->data[uIndex];
  return pPeer;
}

// -----[ bgp_peers_for_each ]---------------------------------------
int bgp_peers_for_each(bgp_peers_t * pPeers, FArrayForEach fForEach,
		       void * pContext)
{
  return _array_for_each((SArray *) pPeers, fForEach, pContext);
}

// -----[ bgp_peers_enum ]-------------------------------------------
enum_t * bgp_peers_enum(bgp_peers_t * pPeers)
{
  return _array_get_enum((SArray *) pPeers);
}
