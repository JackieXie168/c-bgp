// ==================================================================
// @(#)enum.c
//
// Enumeration functions used by the CLI.
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be), 
//
// @date 27/04/2007
// @lastdate 21/07/2007
// ==================================================================

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <string.h>

#include <bgp/as.h>
#include <net/net_types.h>
#include <net/network.h>
#include <net/node.h>
#include <net/protocol.h>

// -----[ cli_enum_net_nodes ]---------------------------------------
SNetNode * cli_enum_net_nodes(const char * pcText, int state)
{
  static SEnumerator * pEnum= NULL;
  SNetNode * pNode;
  char acNode[16];

  if (state == 0)
    pEnum= trie_get_enum(network_get()->pNodes);
  while (enum_has_next(pEnum)) {
    pNode= *((SNetNode **) enum_get_next(pEnum));

    // Optionally check if prefix matches
    if (pcText != NULL) {
      assert(ip_address_to_string(pNode->tAddr, acNode, sizeof(acNode)) >= 0);
      if (strncmp(pcText, acNode, strlen(pcText)))
	continue;
    }

    return pNode;
  }
  enum_destroy(&pEnum);
  return NULL;
}

// -----[ cli_enum_bgp_routers ]-------------------------------------
SBGPRouter * cli_enum_bgp_routers(const char * pcText, int state)
{
  SNetNode * pNode;
  SNetProtocol * pProtocol;

  while ((pNode= cli_enum_net_nodes(pcText, state++)) != NULL) {

    // Check if node supports BGP
    pProtocol= node_get_protocol(pNode, NET_PROTOCOL_BGP);
    if (pProtocol == NULL)
      continue;

    return (SBGPRouter *) pProtocol->pHandler;
  }
  return NULL;
}

// -----[ cli_enum_net_nodes_addr ]---------------------------------------
/**
 * Enumerate all the nodes.
 */
char * cli_enum_net_nodes_addr(const char * pcText, int state)
{
  SNetNode * pNode= NULL;
  char acNode[16];
  
  while ((pNode= cli_enum_net_nodes(pcText, state++)) != NULL) {
    assert(ip_address_to_string(pNode->tAddr, acNode, sizeof(acNode)) >= 0);
    return strdup(acNode);
  }
  return NULL;
}

// -----[ cli_enum_bgp_routers_addr ]--------------------------------
/**
 * Enumerate all the BGP routers.
 */
char * cli_enum_bgp_routers_addr(const char * pcText, int state)
{
  SBGPRouter * pRouter= NULL;
  char acNode[16];
  
  while ((pRouter= cli_enum_bgp_routers(pcText, state)) != NULL) {
    assert(ip_address_to_string(pRouter->pNode->tAddr, acNode,
				sizeof(acNode)) >= 0);
    return strdup(acNode);
  }
  return NULL;
}

