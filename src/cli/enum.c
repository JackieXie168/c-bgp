// ==================================================================
// @(#)enum.c
//
// Enumeration functions used by the CLI.
//
// These functions are NOT thread-safe and MUST only be called from
// the CLI !!!
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be), 
//
// @date 27/04/2007
// @lastdate 13/03/2008
// ==================================================================

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <string.h>

#include <bgp/as.h>
#include <bgp/peer.h>
#include <net/net_types.h>
#include <net/network.h>
#include <net/node.h>
#include <net/protocol.h>

#define IP4_ADDR_STR_LEN 16

static bgp_router_t * _ctx_bgp_router= NULL;

// -----[ cli_enum_ctx_bgp_router ]----------------------------------
void cli_enum_ctx_bgp_router(bgp_router_t * router)
{
  _ctx_bgp_router= router;
}

// -----[ cli_enum_net_nodes ]---------------------------------------
net_node_t * cli_enum_net_nodes(const char * text, int state)
{
  static enum_t * pEnum= NULL;
  net_node_t * node;
  char str_addr[IP4_ADDR_STR_LEN];

  if (state == 0)
    pEnum= trie_get_enum(network_get_default()->nodes);

  while (enum_has_next(pEnum)) {
    node= *((net_node_t **) enum_get_next(pEnum));

    // Optionally check if prefix matches
    if (text != NULL) {
      assert(ip_address_to_string(node->tAddr, str_addr,
				  sizeof(str_addr)) >= 0);
      if (strncmp(text, str_addr, strlen(text)))
	continue;
    }

    return node;
  }
  enum_destroy(&pEnum);
  return NULL;
}

// -----[ cli_enum_bgp_routers ]-------------------------------------
bgp_router_t * cli_enum_bgp_routers(const char * text, int state)
{
  net_node_t * node;
  net_protocol_t * pProtocol;

  while ((node= cli_enum_net_nodes(text, state++)) != NULL) {

    // Check if node supports BGP
    pProtocol= node_get_protocol(node, NET_PROTOCOL_BGP);
    if (pProtocol == NULL)
      continue;

    return (bgp_router_t *) pProtocol->pHandler;
  }
  return NULL;
}

// -----[ cli_enum_bgp_peers ]---------------------------------------
bgp_peer_t * cli_enum_bgp_peers(const char * pcText, int state)
{
  static unsigned int index= 0;

  if (state == 0)
    index= 0;

  if (_ctx_bgp_router == NULL)
    return NULL;

  assert(index >= 0);

  if (index >= bgp_peers_size(_ctx_bgp_router->pPeers))
    return NULL;
  return bgp_peers_at(_ctx_bgp_router->pPeers, index++);
}

// -----[ cli_enum_net_nodes_addr ]----------------------------------
/**
 * Enumerate all the nodes.
 */
char * cli_enum_net_nodes_addr(const char * text, int state)
{
  net_node_t * node= NULL;
  char str_addr[IP4_ADDR_STR_LEN];
  
  while ((node= cli_enum_net_nodes(text, state++)) != NULL) {
    assert(ip_address_to_string(node->tAddr, str_addr, sizeof(str_addr)) >= 0);
    return strdup(str_addr);
  }
  return NULL;
}

// -----[ cli_enum_bgp_routers_addr ]--------------------------------
/**
 * Enumerate all the BGP routers.
 */
char * cli_enum_bgp_routers_addr(const char * text, int state)
{
  bgp_router_t * router= NULL;
  char str_addr[IP4_ADDR_STR_LEN];
  
  while ((router= cli_enum_bgp_routers(text, state++)) != NULL) {
    assert(ip_address_to_string(router->pNode->tAddr, str_addr,
				sizeof(str_addr)) >= 0);
    return strdup(str_addr);
  }
  return NULL;
}

// -----[ cli_enum_bgp_peers_addr ]----------------------------------
/**
 * Enumerate all the BGP peers of one BGP router.
 */
char * cli_enum_bgp_peers_addr(const char * text, int state)
{
  bgp_peer_t * peer= NULL;
  char str_addr[IP4_ADDR_STR_LEN];

  while ((peer= cli_enum_bgp_peers(text, state++)) != NULL) {
    assert(ip_address_to_string(peer->tAddr, str_addr, sizeof(str_addr)) >= 0);

    // Optionally check if prefix matches
    if ((text != NULL) && (strncmp(text, str_addr, strlen(text))))
	continue;

    return strdup(str_addr);
  }
  return NULL;
}
