// ==================================================================
// @(#)net.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 15/07/2003
// @lastdate 05/04/2005
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <string.h>

#include <libgds/cli_ctx.h>
#include <cli/common.h>
#include <cli/net.h>
#include <cli/net_ospf.h>
#include <net/ntf.h>
#include <net/prefix.h>
#include <net/subnet.h>
#include <libgds/log.h>
#include <net/icmp.h>
#include <net/igp.h>
#include <net/network.h>
#include <net/net_path.h>
#include <net/routing.h>
#include <net/ospf.h>
#include <net/ospf_rt.h>
#include <ui/rl.h>

// ----- cli_net_node_by_addr ---------------------------------------
/**
 *
 */
SNetNode * cli_net_node_by_addr(char * pcAddr)
{
  net_addr_t tAddr;
  char * pcEndPtr;

  if (ip_string_to_address(pcAddr, &pcEndPtr, &tAddr) || (*pcEndPtr != 0))
    return NULL;
  return network_find_node(network_get(), tAddr);
}

// ----- cli_net_subnet_by_prefix ---------------------------------------
/**
 *
 */
SNetSubnet * cli_net_subnet_by_prefix(char * pcAddr)
{
  SPrefix sPrefix;
  char * pcEndPtr;

  if (ip_string_to_prefix(pcAddr, &pcEndPtr, &sPrefix) || (*pcEndPtr != 0))
    return NULL;
  return network_find_subnet(network_get(), sPrefix);
}

// ----- cli_net_add_node -------------------------------------------
int cli_net_add_node(SCliContext * pContext, STokens * pTokens)
{
  net_addr_t tAddr;
  char * pcEndPtr;

  if (ip_string_to_address(tokens_get_string_at(pTokens, 0),
			   &pcEndPtr, &tAddr) ||
      (*pcEndPtr != 0))
    return CLI_ERROR_COMMAND_FAILED;
  if (network_find_node(network_get(), tAddr) != NULL) {
    LOG_SEVERE("Error: could not add node (already exists)\n");
    return CLI_ERROR_COMMAND_FAILED;
  }
  if (network_add_node(network_get(), node_create(tAddr))) {
    LOG_SEVERE("Error: could not add node (unknown reason)\n");
    return CLI_ERROR_COMMAND_FAILED;
  }
  return CLI_SUCCESS;
}

// ----- cli_net_add_subnet -------------------------------------------
/**
 * context: {}
 * tokens: {prefix, type}
 */
int cli_net_add_subnet(SCliContext * pContext, STokens * pTokens)
{
  SPrefix sSubnetPrefix;
  char * pcEndPtr;
  char * pcType;
  uint8_t uType;

  if (ip_string_to_prefix(tokens_get_string_at(pTokens, 0),
			   &pcEndPtr, &sSubnetPrefix) ||
      (*pcEndPtr != 0))
    return CLI_ERROR_COMMAND_FAILED;
  if (network_find_subnet(network_get(), sSubnetPrefix) != NULL) {
    LOG_SEVERE("Error: could not add subnet (already exists)\n");
    return CLI_ERROR_COMMAND_FAILED;
  }
  
  pcType = tokens_get_string_at(pTokens, 1);
  
  if (strcmp(pcType, "transit") == 0)
    uType = NET_SUBNET_TYPE_TRANSIT;
  else if (strcmp(pcType, "stub") == 0)
    uType = NET_SUBNET_TYPE_STUB;
  else {
    LOG_SEVERE("Error: wrong subnet type\n");
    return CLI_ERROR_COMMAND_FAILED;
  }
     
  if (network_add_subnet(network_get(), 
                    subnet_create(sSubnetPrefix.tNetwork, sSubnetPrefix.uMaskLen, uType)) != 0) {
    LOG_SEVERE("Error: could not add subnet (unknown reason)\n");
    return CLI_ERROR_COMMAND_FAILED;
  }
  return CLI_SUCCESS;
}

// ----- cli_net_add_link -------------------------------------------
/**
 * context: {}
 * tokens: {addr-src, prefix-dst, delay}
 */
int cli_net_add_link(SCliContext * pContext, STokens * pTokens)
{
  SNetNode * pNodeSrc, * pNodeDst = NULL;
  SNetSubnet * pSubnetDst;
  unsigned int uDelay;
  char * pcNodeSrcAddr, * pcVertexDstPrefix; 
  SNetDest * pDest;
  
  pcNodeSrcAddr= tokens_get_string_at(pTokens, 0);
  pNodeSrc= cli_net_node_by_addr(pcNodeSrcAddr);
  
  if (pNodeSrc == NULL) {
    LOG_SEVERE("Error: could not find node \"%s\"\n", pcNodeSrcAddr);
    return CLI_ERROR_COMMAND_FAILED;
  }
  
  //Destination can be a node (router) or a subnet
  pDest = (SNetDest *) MALLOC(sizeof(SNetDest));
  
  pcVertexDstPrefix= tokens_get_string_at(pTokens, 1);
  ip_string_to_dest(pcVertexDstPrefix, pDest);
  
  if (pDest->tType == NET_DEST_ADDRESS) {
    pNodeDst= cli_net_node_by_addr(pcVertexDstPrefix);
    if (pNodeDst == NULL) {
      LOG_SEVERE("Error: could not find node \"%s\"\n", pcVertexDstPrefix);
      FREE(pDest);
      return CLI_ERROR_CTX_CREATE;
    }
    
    if (tokens_get_uint_at(pTokens, 2, &uDelay)) {
      FREE(pDest);
      return CLI_ERROR_COMMAND_FAILED;
    }
    if (node_add_link(pNodeSrc, pNodeDst, (net_link_delay_t) uDelay, 1) < 0) {
      LOG_SEVERE("Error: could not add link (already exists)\n");
      FREE(pDest);
      return CLI_ERROR_COMMAND_FAILED;
    }
  }
  else if (pDest->tType == NET_DEST_PREFIX) {

    pSubnetDst= cli_net_subnet_by_prefix(pcVertexDstPrefix);
//     LOG_SEVERE("after: cli_net_subnet_by_prefix \"%s\"\n", pcVertexDstPrefix);
    if (pSubnetDst == NULL) {
      LOG_SEVERE("Error: could not find subnet \"%s\"\n", pcVertexDstPrefix);
      FREE(pDest);
      return CLI_ERROR_CTX_CREATE;
    }
//     LOG_SEVERE("call tokens_get_uint_at\n");
    if (tokens_get_uint_at(pTokens, 2, &uDelay)) {
      FREE(pDest);
      return CLI_ERROR_COMMAND_FAILED;
    }
//     LOG_SEVERE("/*/*call*/*/ node_add_link_toSubnet\n");
    if (node_add_link_toSubnet(pNodeSrc, pSubnetDst, (net_link_delay_t) uDelay, 1) < 0) {
      LOG_SEVERE("Error: could not add link (already exists)\n");
      FREE(pDest);
      return CLI_ERROR_COMMAND_FAILED;
    }
  }
  
  FREE(pDest);
  return CLI_SUCCESS;
}

// ----- cli_ctx_create_net_link ------------------------------------
int cli_ctx_create_net_link(SCliContext * pContext, void ** ppItem)
{
  SNetNode * pNodeSrc, * pNodeDst;
  SNetSubnet * pSubnetDst;
  SNetDest * pDest = (SNetDest *)MALLOC(sizeof(SNetDest)); //dest can be a subnet or a node
  char * pcNodeSrcAddr, * pcVertexDstPrefix; 
  

  pcNodeSrcAddr= tokens_get_string_at(pContext->pTokens, 0);
  pNodeSrc= cli_net_node_by_addr(pcNodeSrcAddr);
  if (pNodeSrc == NULL) {
    LOG_SEVERE("Error: unable to find node \"%s\"\n", pcNodeSrcAddr);
    FREE(pDest);
    return CLI_ERROR_CTX_CREATE;
  }
  
  pcVertexDstPrefix= tokens_get_string_at(pContext->pTokens, 1);
  if (ip_string_to_dest(pcVertexDstPrefix, pDest) < 0 ||
      pDest->tType == NET_DEST_ANY) {
    LOG_SEVERE("Error: destination id is wrong \"%s\"\n", pcVertexDstPrefix);
    FREE(pDest);
    return CLI_ERROR_CTX_CREATE;
  }
  
  if (pDest->tType == NET_DEST_ADDRESS) {
    pNodeDst= cli_net_node_by_addr(pcVertexDstPrefix);
    if (pNodeDst == NULL) {
      LOG_SEVERE("Error: unable to find node \"%s\"\n", pcVertexDstPrefix);
      FREE(pDest);
      return CLI_ERROR_CTX_CREATE;
    }
    *ppItem= node_find_link_to_router(pNodeSrc, pNodeDst->tAddr);
    if (*ppItem == NULL) {
      LOG_SEVERE("Error: unable to find link \"%s-%s\"\n",
	       pcNodeSrcAddr, pcVertexDstPrefix);
      FREE(pDest);
      return CLI_ERROR_CTX_CREATE;
    }
  }
  else if (pDest->tType == NET_DEST_PREFIX) {
    pSubnetDst= cli_net_subnet_by_prefix(pcVertexDstPrefix);
    if (pSubnetDst == NULL) {
      LOG_SEVERE("Error: unable to find subnet \"%s\"\n", pcVertexDstPrefix);
      FREE(pDest);
      return CLI_ERROR_CTX_CREATE;
    }
    *ppItem= node_find_link_to_subnet(pNodeSrc, pSubnetDst);
    if (*ppItem == NULL) {
      LOG_SEVERE("Error: unable to find link \"%s-%s\"\n",
	       pcNodeSrcAddr, pcVertexDstPrefix);
      FREE(pDest);
      return CLI_ERROR_CTX_CREATE;
    }
  }
  FREE(pDest);
  LOG_DEBUG("cli_ctx_create_net_link end\n");
  return CLI_SUCCESS;
}

// ----- cli_ctx_destroy_net_link -----------------------------------
void cli_ctx_destroy_net_link(void ** ppItem)
{
}

// ----- cli_net_ntf_load -------------------------------------------
/**
 * context: {}
 * tokens: {ntf_file}
 */
int cli_net_ntf_load(SCliContext * pContext, STokens * pTokens)
{
  char * pcFileName;

  // Get name of the NTF file
  pcFileName= tokens_get_string_at(pTokens, 0);

  // Load given NTF file
  if (ntf_load(pcFileName) != NTF_SUCCESS) {
    LOG_SEVERE("Error: unable to load NTF file \"%s\"\n",
	       pcFileName);
    return CLI_ERROR_COMMAND_FAILED;
  }

  return CLI_SUCCESS;
}

// ----- cli_net_node_ipip_enable -----------------------------------
/**
 * context: {node}
 * tokens: {addr}
 */
int cli_net_node_ipip_enable(SCliContext * pContext, STokens * pTokens)
{
  SNetNode * pNode;

  // Get node from context
  pNode= (SNetNode *) cli_context_get_item_at_top(pContext);

  // Enabled IP-in-IP
  if (node_ipip_enable(pNode)) {
    LOG_SEVERE("Error: unexpected error.\n");
    return CLI_ERROR_COMMAND_FAILED;
  }

  return CLI_SUCCESS;
}

// ----- cli_net_node_spfprefix -------------------------------------
/**
 * context: {node}
 * tokens: {addr, prefix}
 */
int cli_net_node_spfprefix(SCliContext * pContext, STokens * pTokens)
{
  SNetNode * pNode;
  char * pcPrefix;
  SPrefix sPrefix;
  char * pcEndChar;

  // Get node from the CLI's context
  pNode= (SNetNode *) cli_context_get_item_at_top(pContext);
  if (pNode == NULL)
    return CLI_ERROR_COMMAND_FAILED;
  
  // Get the prefix
  pcPrefix= tokens_get_string_at(pTokens, 1);
  if (ip_string_to_prefix(pcPrefix, &pcEndChar, &sPrefix) ||
      (*pcEndChar != 0)) {
    LOG_SEVERE("Error: invalid prefix \"%s\"\n", pcPrefix);
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Compute the SPF from the node towards all the nodes in the prefix
  if (igp_compute_prefix(network_get(), pNode, sPrefix))
    return CLI_ERROR_COMMAND_FAILED;

  return CLI_SUCCESS;
}

// ----- cli_net_link_up --------------------------------------------
int cli_net_link_up(SCliContext * pContext, STokens * pTokens)
{
  SNetLink * pLink;

  pLink= (SNetLink *) cli_context_get_item_at_top(pContext);
  if (pLink == NULL)
    return CLI_ERROR_COMMAND_FAILED;
  link_set_state(pLink, NET_LINK_FLAG_UP, 1);
  return CLI_SUCCESS;
}

// ----- cli_net_link_down ------------------------------------------
int cli_net_link_down(SCliContext * pContext, STokens * pTokens)
{
  SNetLink * pLink;

  pLink= (SNetLink *) cli_context_get_item_at_top(pContext);
  if (pLink == NULL)
    return CLI_ERROR_COMMAND_FAILED;
  link_set_state(pLink, NET_LINK_FLAG_UP, 0);
  return CLI_SUCCESS;
}

// ----- cli_net_link_igpweight -------------------------------------
/**
 * context: {link}
 * tokens: {addr-src, addr-dst, igp-weight}
 */
int cli_net_link_igpweight(SCliContext * pContext, STokens * pTokens)
{
  SNetLink * pLink;
  unsigned int uWeight;

  pLink= (SNetLink *) cli_context_get_item_at_top(pContext);
  if (pLink == NULL)
    return CLI_ERROR_COMMAND_FAILED;
  if (tokens_get_uint_at(pTokens, 2, &uWeight) != 0)
    return CLI_ERROR_COMMAND_FAILED;
  link_set_igp_weight(pLink, uWeight);
  return CLI_SUCCESS;
}

// ----- cli_ctx_create_net_node ------------------------------------
int cli_ctx_create_net_node(SCliContext * pContext, void ** ppItem)
{
  char * pcNodeAddr;
  SNetNode * pNode;

  pcNodeAddr= tokens_get_string_at(pContext->pTokens, 0);
  pNode= cli_net_node_by_addr(pcNodeAddr);
  if (pNode == NULL) {
    LOG_SEVERE("Error: unable to find node \"%s\"\n", pcNodeAddr);
    return CLI_ERROR_CTX_CREATE;
  }
  *ppItem= pNode;
  return CLI_SUCCESS;
}

// ----- cli_ctx_destroy_net_node -----------------------------------
void cli_ctx_destroy_net_node(void ** ppItem)
{
}

// ----- cli_ctx_create_net_node ------------------------------------
int cli_ctx_create_net_subnet(SCliContext * pContext, void ** ppItem)
{
  char * pcSubnetPfx;
  SNetSubnet * pSubnet;

  pcSubnetPfx= tokens_get_string_at(pContext->pTokens, 0);
  pSubnet = cli_net_subnet_by_prefix(pcSubnetPfx);
  
  if (pSubnet == NULL) {
    LOG_SEVERE("Error: unable to find subnet \"%s\"\n", pcSubnetPfx);
    return CLI_ERROR_CTX_CREATE;
  }
  *ppItem= pSubnet;
  return CLI_SUCCESS;
}

// ----- cli_ctx_destroy_net_subnet -----------------------------------
void cli_ctx_destroy_net_subnet(void ** ppItem)
{
}

// ----- cli_ctx_create_net_node_link ------------------------------------
int cli_ctx_create_net_node_link(SCliContext * pContext, void ** ppItem)
{
   char * pcPrefix;//, * pcEndPtr;
//   SPrefix sDestPrefix;
  SNetLink * pLink = NULL;
  SNetDest sDest;
  
  SNetNode * pNodeSource = (SNetNode *) cli_context_get_item_at_top(pContext);
  pcPrefix = tokens_get_string_at(pContext->pTokens, 1);
  
  if (ip_string_to_dest(pcPrefix, &sDest)) {
    LOG_SEVERE("Error: invalid destination prefix|address|* \"%s\"\n", pcPrefix);
    return CLI_ERROR_COMMAND_FAILED;
  }

  /* Check that the destination type is adress/prefix */
  if ((sDest.tType == NET_DEST_ANY)) {
    LOG_SEVERE("Error: can not use this destination type with link\n");
    return CLI_ERROR_COMMAND_FAILED;
  } else if (sDest.tType == NET_DEST_ADDRESS) {
     (sDest.uDest).sPrefix.uMaskLen = 32;
  }
 
  pLink= node_find_link(pNodeSource, &((sDest.uDest).sPrefix));
  if (pLink == NULL) {
    LOG_SEVERE("Error: unable to find link \"%s\"\n", pcPrefix);
    return CLI_ERROR_CTX_CREATE;
  }
  *ppItem= pLink;
  return CLI_SUCCESS;
}

// ----- cli_ctx_destroy_net_node_link -----------------------------------
void cli_ctx_destroy_net_node_link(void ** ppItem)
{
}

// ----- cli_net_node_ping ------------------------------------------
/**
 * context: {node}
 * tokens: {addr, addr}
 */
int cli_net_node_ping(SCliContext * pContext, STokens * pTokens)
{
  SNetNode * pNode;
  char * pcDstAddr;
  char * pcEndChar;
  net_addr_t tDstAddr;

 // Get node from the CLI'scontext
  pNode= (SNetNode *) cli_context_get_item_at_top(pContext);
  if (pNode == NULL)
    return CLI_ERROR_COMMAND_FAILED;

  // Get destination address
  pcDstAddr= tokens_get_string_at(pContext->pTokens, 1);
  if (ip_string_to_address(pcDstAddr, &pcEndChar, &tDstAddr) ||
      (*pcEndChar != 0)) {
    LOG_SEVERE("Error: invalid address \"%s\"\n",
	       pcDstAddr);
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Send ICMP echo request
  if (icmp_echo_request(pNode, tDstAddr)) {
    LOG_SEVERE("Error: ICMP echo request failed\n");
    return CLI_ERROR_COMMAND_FAILED;
  }
  return CLI_SUCCESS;
}

// ----- cli_net_node_recordroute -----------------------------------
/**
 * context: {node}
 * tokens: {addr, addr}
 */
int cli_net_node_recordroute(SCliContext * pContext, STokens * pTokens)
{
  SNetNode * pNode;
  char * pcDest;
  SNetDest sDest;

 // Get node from the CLI'scontext
  pNode= (SNetNode *) cli_context_get_item_at_top(pContext);
  if (pNode == NULL)
    return CLI_ERROR_COMMAND_FAILED;

  // Get destination address
  pcDest= tokens_get_string_at(pTokens, 1);
  if (ip_string_to_dest(pcDest, &sDest)) {
    LOG_SEVERE("Error: invalid prefix|address|* \"%s\"\n", pcDest);
    return CLI_ERROR_COMMAND_FAILED;
  }

  /* Check that the destination type is adress/prefix */
  if ((sDest.tType != NET_DEST_ADDRESS) &&
      (sDest.tType != NET_DEST_PREFIX)) {
    LOG_SEVERE("Error: can not use this destination type with record-route\n");
    return CLI_ERROR_COMMAND_FAILED;
  }

  node_dump_recorded_route(stdout, pNode, sDest, 0, 0);

  return CLI_SUCCESS;
}

// ----- cli_net_node_recordroutedelay ------------------------------
/**
 * context: {node}
 * tokens: {addr, addr}
 */
int cli_net_node_recordroutedelay(SCliContext * pContext,
				  STokens * pTokens)
{
  SNetNode * pNode;
  char * pcDest;
  SNetDest sDest;

 // Get node from the CLI'scontext
  pNode= (SNetNode *) cli_context_get_item_at_top(pContext);
  if (pNode == NULL)
    return CLI_ERROR_COMMAND_FAILED;

  // Get destination address
  pcDest= tokens_get_string_at(pTokens, 1);
  if (ip_string_to_dest(pcDest, &sDest)) {
    LOG_SEVERE("Error: invalid prefix|address|* \"%s\"\n", pcDest);
    return CLI_ERROR_COMMAND_FAILED;
  }

  /* Check that the destination type is adress/prefix */
  if ((sDest.tType != NET_DEST_ADDRESS) &&
      (sDest.tType != NET_DEST_PREFIX)) {
    LOG_SEVERE("Error: can not use this destination type with record-route\n");
    return CLI_ERROR_COMMAND_FAILED;
  }

  node_dump_recorded_route(stdout, pNode, sDest, 1, 0);

  return CLI_SUCCESS;
}

// ----- cli_net_node_recordroutedeflection ------------------------------
/**
 * context: {node}
 * tokens: {addr, addr}
 */
int cli_net_node_recordroutedeflection(SCliContext * pContext,
				  STokens * pTokens)
{
  SNetNode * pNode;
  char * pcDest;
  SNetDest sDest;

 // Get node from the CLI'scontext
  pNode= (SNetNode *) cli_context_get_item_at_top(pContext);
  if (pNode == NULL)
    return CLI_ERROR_COMMAND_FAILED;

  // Get destination address
  pcDest= tokens_get_string_at(pTokens, 1);
  if (ip_string_to_dest(pcDest, &sDest)) {
    LOG_SEVERE("Error: invalid prefix|address|* \"%s\"\n", pcDest);
    return CLI_ERROR_COMMAND_FAILED;
  }

  /* Check that the destination type is adress/prefix */
  if ((sDest.tType != NET_DEST_ADDRESS) &&
      (sDest.tType != NET_DEST_PREFIX)) {
    LOG_SEVERE("Error: can not use this destination type with record-route\n");
    return CLI_ERROR_COMMAND_FAILED;
  }

  node_dump_recorded_route(stdout, pNode, sDest, 0, 1);

  return CLI_SUCCESS;
}

// ----- cli_net_node_route_add -------------------------------------
/**
 * This function adds a new static route to the given node.
 *
 * context: {node}
 * tokens: {addr, prefix, next-hop, weight}
 */
int cli_net_node_route_add(SCliContext * pContext, STokens * pTokens)
{
  SNetNode * pNode;
  char * pcEndChar;
  char * pcPrefix;
  SPrefix sPrefix;
  char * pcNextHopAddr;
  net_addr_t tNextHopAddr;
  unsigned long ulWeight;

  // Get node from the CLI'scontext
  pNode= (SNetNode *) cli_context_get_item_at_top(pContext);
  if (pNode == NULL)
    return CLI_ERROR_COMMAND_FAILED;
  
  // Get the route's prefix
  pcPrefix= tokens_get_string_at(pTokens, 1);
  if (ip_string_to_prefix(pcPrefix, &pcEndChar, &sPrefix) ||
      (*pcEndChar != 0)) {
    LOG_SEVERE("Error: invalid prefix \"%s\"\n", pcPrefix);
    return CLI_ERROR_COMMAND_FAILED;
  }
  
  // Get the next-hop's address
  pcNextHopAddr= tokens_get_string_at(pTokens, 2);
  if (ip_string_to_address(pcNextHopAddr, &pcEndChar, &tNextHopAddr) ||
      (*pcEndChar != 0)) {
    LOG_SEVERE("Error: invalid next-hop address \"%s\"\n",
	       pcNextHopAddr);
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Get the weight
  if (tokens_get_ulong_at(pTokens, 3, &ulWeight)) {
    LOG_SEVERE("Error: invalid weight\n");
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Add the route
  if (node_rt_add_route(pNode, sPrefix, tNextHopAddr,
			ulWeight, NET_ROUTE_STATIC)) {
    LOG_SEVERE("Error: could not add static route\n");
    return CLI_ERROR_COMMAND_FAILED;
  }

  return CLI_SUCCESS;
}

// ----- cli_net_node_route_del -------------------------------------
/**
 * This function removes a static route from the given node.
 *
 * context: {node}
 * tokens: {addr, prefix, next-hop, weight}
 */
int cli_net_node_route_del(SCliContext * pContext, STokens * pTokens)
{
  SNetNode * pNode;
  char * pcEndChar;
  char * pcPrefix;
  SPrefix sPrefix;
  char * pcNextHopAddr;
  net_addr_t tNextHopAddr;
  net_addr_t * pNextHopAddr= &tNextHopAddr;

  // Get node from the CLI'scontext
  pNode= (SNetNode *) cli_context_get_item_at_top(pContext);
  if (pNode == NULL)
    return CLI_ERROR_COMMAND_FAILED;
  
  // Get the route's prefix
  pcPrefix= tokens_get_string_at(pTokens, 1);
  if (ip_string_to_prefix(pcPrefix, &pcEndChar, &sPrefix) ||
      (*pcEndChar != 0)) {
    LOG_SEVERE("Error: invalid prefix \"%s\"\n", pcPrefix);
    return CLI_ERROR_COMMAND_FAILED;
  }
  
  // Get the next-hop's address
  pcNextHopAddr= tokens_get_string_at(pTokens, 2);
  if (!strcmp(pcNextHopAddr, "*")) {
    pNextHopAddr= NULL;
  } else if (ip_string_to_address(pcNextHopAddr, &pcEndChar, pNextHopAddr) ||
	     (*pcEndChar != 0)) {
    LOG_SEVERE("Error: invalid next-hop address \"%s\"\n",
	       pcNextHopAddr);
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Remove the route
  if (node_rt_del_route(pNode, &sPrefix, pNextHopAddr, NET_ROUTE_STATIC)) {
    LOG_SEVERE("Error: could not remove static route\n");
    return CLI_ERROR_COMMAND_FAILED;
  }

  return CLI_SUCCESS;
}



// ----- cli_net_node_show_links ------------------------------------
/**
 * context: {node}
 * tokens: {addr}
 */
int cli_net_node_show_links(SCliContext * pContext, STokens * pTokens)
{
  SNetNode * pNode;

  // Get node from the CLI'scontext
  pNode= (SNetNode *) cli_context_get_item_at_top(pContext);
  if (pNode == NULL)
    return CLI_ERROR_COMMAND_FAILED;

  // Dump list of direct links
  node_links_dump(stdout, pNode);
  
  return CLI_SUCCESS;
}

// ----- cli_net_node_show_rt ---------------------------------------
/**
 * context: {node}
 * tokens: {addr}
 */
int cli_net_node_show_rt(SCliContext * pContext, STokens * pTokens)
{
  SNetNode * pNode;
  char * pcPrefix;
  SNetDest sDest;

  // Get node from the CLI'scontext
  pNode= (SNetNode *) cli_context_get_item_at_top(pContext);
  if (pNode == NULL)
    return CLI_ERROR_COMMAND_FAILED;

  // Get the prefix/address/*
  pcPrefix= tokens_get_string_at(pTokens, 1);
  if (ip_string_to_dest(pcPrefix, &sDest)) {
    LOG_SEVERE("Error: invalid prefix|address|* \"%s\"\n", pcPrefix);
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Dump routing table
//   node_rt_dump(stdout, pNode, sDest);
// ----- ospf_node_rt_dump ------------------------------------------------------------------
/**  Option:
  *  OSPF_RT_OPTION_SORT_AREA : dump routing table grouping routes by area
  */
  ospf_node_rt_dump(stdout, pNode, OSPF_RT_OPTION_SORT_AREA | OSPF_RT_OPTION_SORT_PATH_TYPE);
  return CLI_SUCCESS;
}

// ----- cli_net_node_tunnel_add ------------------------------------
/**
 * context: {node}
 * tokens: {addr, end-point}
 */
int cli_net_node_tunnel_add(SCliContext * pContext, STokens * pTokens)
{
  SNetNode * pNode;
  char * pcEndPointAddr;
  net_addr_t tEndPointAddr;
  char * pcEndChar;

  // Get node from context
  pNode= (SNetNode *) cli_context_get_item_at_top(pContext);

  // Get tunnel end-point
  pcEndPointAddr= tokens_get_string_at(pTokens, 1);
  if (ip_string_to_address(pcEndPointAddr, &pcEndChar, &tEndPointAddr) ||
      (*pcEndChar != 0)) {
    LOG_SEVERE("Error: invalid end-point address \"%s\"\n",
	       pcEndPointAddr);
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Add tunnel
  if (node_add_tunnel(pNode, tEndPointAddr)) {
    LOG_SEVERE("Error: unexpected error\n");
    return CLI_ERROR_COMMAND_FAILED;
  }

  return CLI_SUCCESS;
}

// ----- cli_net_options_maxhops ------------------------------------
/**
 * Change the maximum number of hops used in IP-level
 * record-routes. The maximum number of hops must be in the range
 * [0, 255].
 *
 * context: {}
 * tokens: {maxhops}
 */
int cli_net_options_maxhops(SCliContext * pContext, STokens * pTokens)
{
  unsigned int uMaxHops;

  if (tokens_get_uint_at(pTokens, 0, &uMaxHops)) {
    LOG_SEVERE("Error: invalid value for max-hops (%s)\n",
	       tokens_get_string_at(pTokens, 0));
    return CLI_ERROR_COMMAND_FAILED;
  }

  if (uMaxHops > 255) {
    LOG_SEVERE("Error: maximum number of hops is 255\n");
    return CLI_ERROR_COMMAND_FAILED;
  }

  NET_OPTIONS_MAX_HOPS= uMaxHops;

  return CLI_SUCCESS;
}

// ----- cli_net_options_igpinter -----------------------------------
/**
 * Configure the "IGP", i.e. the SPT computation, so that it also
 * consider as destinations the nodes at the end of interdomain
 * links.
 *
 * context: {}
 * tokens: {state}
 */
int cli_net_options_igpinter(SCliContext * pContext, STokens * pTokens)
{
  if (!strcmp(tokens_get_string_at(pTokens, 0), "on")) {
    NET_OPTIONS_IGP_INTER= 1;
  } else if (!strcmp(tokens_get_string_at(pTokens, 0), "off")) {
    NET_OPTIONS_IGP_INTER= 0;
  } else {
    LOG_SEVERE("Error: invalid value\n");
    return CLI_ERROR_COMMAND_FAILED;
  }

  return CLI_SUCCESS;
}

// ----- cli_net_show_nodes -----------------------------------------
/**
 * Display all nodes matching the given criterion, which is a prefix
 * or an asterisk (meaning "all nodes").
 *
 * context: {}
 * tokens: {prefix}
 */
int cli_net_show_nodes(SCliContext * pContext, STokens * pTokens)
{
  return CLI_SUCCESS;
}

// ----- cli_net_show_nodes -----------------------------------------
/**
 * Display all nodes matching the given criterion, which is a prefix
 * or an asterisk (meaning "all nodes").
 *
 * context: {}
 * tokens: {}
 */
int cli_net_show_subnets(SCliContext * pContext, STokens * pTokens)
{
  network_dump_subnets(stdout, network_get());
  return CLI_SUCCESS;
}

// ----- cli_register_net_add ---------------------------------------
int cli_register_net_add(SCliCmds * pCmds)
{
  SCliCmds * pSubCmds;
  SCliParams * pParams;

  pSubCmds= cli_cmds_create();
  pParams= cli_params_create();
  cli_params_add(pParams, "<addr>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("node", cli_net_add_node,
					NULL, pParams));
					
					
  pParams= cli_params_create();
  cli_params_add(pParams, "<prefix>", NULL);
  cli_params_add(pParams, "<transit|stub>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("subnet", cli_net_add_subnet,
					NULL, pParams));
  pParams= cli_params_create();
  cli_params_add(pParams, "<addr-src>", NULL);
  cli_params_add(pParams, "<addr-dst>", NULL);
  cli_params_add(pParams, "<delay>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("link", cli_net_add_link,
					NULL, pParams));
  return cli_cmds_add(pCmds, cli_cmd_create("add", NULL, pSubCmds, NULL));
}

// ----- cli_register_net_link --------------------------------------
int cli_register_net_link(SCliCmds * pCmds)
{
  SCliCmds * pSubCmds;
  SCliParams * pParams;

  pSubCmds= cli_cmds_create();
  cli_cmds_add(pSubCmds, cli_cmd_create("up", cli_net_link_up,
					NULL, NULL));
  cli_cmds_add(pSubCmds, cli_cmd_create("down", cli_net_link_down,
					NULL, NULL));
  pParams= cli_params_create();
  cli_params_add(pParams, "", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("igp-weight", cli_net_link_igpweight,
					NULL, pParams));
  pParams= cli_params_create();
  cli_params_add(pParams, "<addr-src>", NULL);
  cli_params_add(pParams, "<addr-dst>", NULL);
  return cli_cmds_add(pCmds, cli_cmd_create_ctx("link",
						cli_ctx_create_net_link,
						cli_ctx_destroy_net_link,
						pSubCmds, pParams));
}

// ----- cli_register_net_ntf ---------------------------------------
int cli_register_net_ntf(SCliCmds * pCmds)
{
  SCliCmds * pSubCmds;
  SCliParams * pParams;

  pSubCmds= cli_cmds_create();
  pParams= cli_params_create();
#ifdef _FILENAME_COMPLETION_FUNCTION
  cli_params_add2(pParams, "<filename>", NULL,
		  _FILENAME_COMPLETION_FUNCTION);
#else
  cli_params_add(pParams, "<filename>", NULL);
#endif
  cli_cmds_add(pSubCmds, cli_cmd_create("load",
					cli_net_ntf_load,
					NULL, pParams));
  return cli_cmds_add(pCmds, cli_cmd_create_ctx("ntf", NULL, NULL,
						pSubCmds, NULL));
}

// ----- cli_register_net_node_show ---------------------------------
int cli_register_net_node_show(SCliCmds * pCmds)
{
  SCliCmds * pSubCmds;
  SCliParams * pParams;

  pSubCmds= cli_cmds_create();
  cli_cmds_add(pSubCmds, cli_cmd_create("links",
					cli_net_node_show_links,
					NULL, NULL));
  pParams= cli_params_create();
  cli_params_add(pParams, "<prefix|address|*>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("rt",
					cli_net_node_show_rt,
					NULL, pParams));
  return cli_cmds_add(pCmds, cli_cmd_create("show", NULL,
					    pSubCmds, NULL));
}

// ----- cli_register_net_node_route --------------------------------
void cli_register_net_node_route(SCliCmds * pCmds)
{
  SCliCmds * pSubCmds= cli_cmds_create();
  SCliParams * pParams;

  pParams= cli_params_create();
  cli_params_add(pParams, "<prefix>", NULL);
  cli_params_add(pParams, "<next-hop>", NULL);
  cli_params_add(pParams, "<weight>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("add",
					cli_net_node_route_add,
					NULL, pParams));
  pParams= cli_params_create();
  cli_params_add(pParams, "<prefix>", NULL);
  cli_params_add(pParams, "<next-hop>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("del",
					cli_net_node_route_del,
					NULL, pParams));
  cli_cmds_add(pCmds, cli_cmd_create("route", NULL, pSubCmds, NULL));
}




// ----- cli_register_net_node_link --------------------------------
int cli_register_net_node_link(SCliCmds * pCmds)
{
  SCliCmds * pSubCmds= cli_cmds_create();
  SCliParams * pParams;

  cli_register_net_node_link_ospf(pSubCmds);
  
  pParams= cli_params_create();
  cli_params_add(pParams, "<prefix>", NULL);			
//   cli_cmds_add(pCmds, cli_cmd_create("link", NULL, pSubCmds, pParams));
  return cli_cmds_add(pCmds, cli_cmd_create_ctx("link",
						cli_ctx_create_net_node_link,
						cli_ctx_destroy_net_node_link,
						pSubCmds, pParams));
}

// ----- cli_register_net_node_tunnel -------------------------------
int cli_register_net_node_tunnel(SCliCmds * pCmds)
{
  SCliCmds * pSubCmds= cli_cmds_create();
  SCliParams * pParams;

  pParams= cli_params_create();
  cli_params_add(pParams, "<end-point>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("add",
					cli_net_node_tunnel_add,
					NULL, pParams));
  return cli_cmds_add(pCmds, cli_cmd_create("tunnel", NULL,
					   pSubCmds, NULL));
}

// ----- cli_register_net_node --------------------------------------
int cli_register_net_node(SCliCmds * pCmds)
{
  SCliCmds * pSubCmds;
  SCliParams * pParams;

  pSubCmds= cli_cmds_create();
  
  
  cli_register_net_node_link(pSubCmds);
  cli_register_net_node_route(pSubCmds);
  
  cli_cmds_add(pSubCmds, cli_cmd_create("ipip-enable",
					cli_net_node_ipip_enable,
					NULL, NULL));
  
  pParams= cli_params_create();
  cli_params_add(pParams, "<address>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("ping",
					cli_net_node_ping,
					NULL, pParams));
  pParams= cli_params_create();
  cli_params_add(pParams, "<address|prefix>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("record-route",
					cli_net_node_recordroute,
					NULL, pParams));
  pParams= cli_params_create();
  cli_params_add(pParams, "<address|prefix>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("record-route-delay",
					cli_net_node_recordroutedelay,
					NULL, pParams));

//sta : Cli command to check deflection in a network.
  pParams= cli_params_create();
  cli_params_add(pParams, "<address|prefix>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("record-route-deflection",
					cli_net_node_recordroutedeflection,
					NULL, pParams));

  pParams= cli_params_create();
  cli_params_add(pParams, "<prefix>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("spf-prefix",
					cli_net_node_spfprefix,
					NULL, pParams));
  cli_register_net_node_show(pSubCmds);
  cli_register_net_node_tunnel(pSubCmds);
  cli_register_net_node_ospf(pSubCmds);
  pParams= cli_params_create();
  cli_params_add(pParams, "<addr>", NULL);
  return cli_cmds_add(pCmds, cli_cmd_create_ctx("node",
						cli_ctx_create_net_node,
						cli_ctx_destroy_net_node,
						pSubCmds, pParams));
}

// ----- cli_register_net_subnet --------------------------------------
int cli_register_net_subnet(SCliCmds * pCmds)
{
  SCliCmds * pSubCmds;
  SCliParams * pParams;

  pSubCmds= cli_cmds_create();
  
  cli_register_net_subnet_ospf(pSubCmds);
  pParams= cli_params_create();
  cli_params_add(pParams, "<prefix>", NULL);
  return cli_cmds_add(pCmds, cli_cmd_create_ctx("subnet",
						cli_ctx_create_net_subnet,
						cli_ctx_destroy_net_subnet,
						pSubCmds, pParams));
}

// ----- cli_register_net_options -----------------------------------
/**
 *
 */
int cli_register_net_options(SCliCmds * pCmds)
{
  SCliCmds * pSubCmds;
  SCliParams * pParams;

  pSubCmds= cli_cmds_create();
  pParams= cli_params_create();
  cli_params_add(pParams, "<max-hops>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("max-hops",
					cli_net_options_maxhops,
					NULL, pParams));
  pParams= cli_params_create();
  cli_params_add(pParams, "<state>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("igp-inter",
					cli_net_options_igpinter,
					NULL, pParams));
  return cli_cmds_add(pCmds, cli_cmd_create_ctx("options",
						NULL, NULL,
						pSubCmds, NULL));
}

// ----- cli_register_net_show --------------------------------------
/**
 *
 */
int cli_register_net_show(SCliCmds * pCmds)
{
  SCliCmds * pSubCmds;
  SCliParams * pParams;

  pSubCmds= cli_cmds_create();
  pParams= cli_params_create();
  cli_params_add(pParams, "<prefix|*>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("nodes", cli_net_show_nodes,
					NULL, pParams));
  
  cli_cmds_add(pSubCmds, cli_cmd_create("subnets", cli_net_show_subnets,
					NULL, NULL));
  return cli_cmds_add(pCmds, cli_cmd_create("show", NULL,
					    pSubCmds, NULL));
}


// ----- cli_register_net -------------------------------------------
/**
 *
 */
int cli_register_net(SCli * pCli)
{
  SCliCmds * pCmds;

  pCmds= cli_cmds_create();
  cli_register_net_add(pCmds);
  cli_register_net_link(pCmds);
  cli_register_net_ntf(pCmds);
  cli_register_net_node(pCmds);
  cli_register_net_subnet(pCmds);
  cli_register_net_options(pCmds);
  cli_register_net_show(pCmds);
  cli_register_net_ospf(pCmds);
  cli_register_cmd(pCli, cli_cmd_create("net", NULL, pCmds, NULL));
  return 0;
}
