// ==================================================================
// @(#)peer-list.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 10/03/2008
// $Id: peer-list.c,v 1.2 2009-03-24 14:28:25 bqu Exp $
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <net/error.h>

#include <bgp/peer.h>
#include <bgp/peer-list.h>

// -----[ _bgp_peers_compare ]---------------------------------------
static int _bgp_peers_compare(const void * item1,
			      const void * item2,
			      unsigned int elt_size)
{
  bgp_peer_t * peer1= *((bgp_peer_t **) item1);
  bgp_peer_t * peer2= *((bgp_peer_t **) item2);

  if (peer1->addr < peer2->addr)
    return -1;
  else if (peer1->addr > peer2->addr)
    return 1;
  else
    return 0;
}

// -----[ _bgp_peers_destroy ]---------------------------------------
static void _bgp_peers_destroy(void * item, const void * ctx)
{
  bgp_peer_t * peer= *((bgp_peer_t **) item);
  bgp_peer_destroy(&peer);
}

// -----[ bgp_peers_create ]-----------------------------------------
bgp_peers_t * bgp_peers_create()
{
  return ptr_array_create(ARRAY_OPTION_SORTED |
			  ARRAY_OPTION_UNIQUE,
			  _bgp_peers_compare,
			  _bgp_peers_destroy,
			  NULL);
}

// -----[ bgp_peers_destroy ]----------------------------------------
void bgp_peers_destroy(bgp_peers_t ** peers_ref)
{
  ptr_array_destroy(peers_ref);
}

// -----[ bgp_peers_add ]--------------------------------------------
net_error_t bgp_peers_add(bgp_peers_t * peers, bgp_peer_t * peer)
{
  if (ptr_array_add(peers, &peer) < 0)
    return EBGP_PEER_DUPLICATE;
  return ESUCCESS;
}

// -----[ bgp_peers_find ]-------------------------------------------
bgp_peer_t * bgp_peers_find(bgp_peers_t * peers, net_addr_t addr)
{
  unsigned int index;
  bgp_peer_t tDummyPeer= { .addr= addr }, * pDummyPeer= &tDummyPeer;

  if (ptr_array_sorted_find_index(peers, &pDummyPeer, &index) != -1)
    return (bgp_peer_t *) peers->data[index];
  return NULL;
}

// -----[ bgp_peers_for_each ]---------------------------------------
int bgp_peers_for_each(bgp_peers_t * peers, gds_array_foreach_f foreach,
		       void * ctx)
{
  return _array_for_each((array_t *) peers, foreach, ctx);
}

// -----[ bgp_peers_enum ]-------------------------------------------
gds_enum_t * bgp_peers_enum(bgp_peers_t * peers)
{
  return _array_get_enum((array_t *) peers);
}



// -----[ bgp_peers_at ]---------------------------------------------
 bgp_peer_t * bgp_peers_at(bgp_peers_t * peers,
					unsigned int index) {
  return (bgp_peer_t *) peers->data[index];
}
// -----[ bgp_peers_size ]-------------------------------------------
 unsigned int bgp_peers_size(bgp_peers_t * peers) {
  return ptr_array_length(peers);
}
