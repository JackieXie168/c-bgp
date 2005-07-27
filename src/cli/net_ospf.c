// ==================================================================
// @(#)net_ospf.c
//
// @author Stefano Iasi (stefanoia@tin.it)
// @date 15/07/2003
// @lastdate 05/04/2005
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <string.h>

#include <libgds/cli_ctx.h>
#include <cli/common.h>
#include <cli/net_ospf.h>
#include <net/ospf.h>
#include <net/igp_domain.h>
#include <net/net_types.h>
#include <ui/rl.h>
#include <libgds/log.h>



// ----- cli_net_node_ospf_area ------------------------------------
/**
 * context: {node}
 * tokens: {addr, area}
 */
int cli_net_node_ospf_area(SCliContext * pContext, STokens * pTokens)
{
  SNetNode * pNode;
  ospf_area_t uArea;

  // Get node from context
  pNode= (SNetNode *) cli_context_get_item_at_top(pContext);

  // Add ospf area to node
  tokens_get_uint_at(pTokens, 1, &uArea);
  if (node_add_OSPFArea(pNode, uArea) < 0) {
    LOG_SEVERE("Error: unexpected error\n");
    return CLI_ERROR_COMMAND_FAILED;
  }

  return CLI_SUCCESS;
}

// ----- cli_net_node_ospf_domain ------------------------------------
/**
 * context: {node}
 * tokens: {addr, igp-domain-id}
 */
int cli_net_node_ospf_domain(SCliContext * pContext, STokens * pTokens)
{
  SNetNode * pNode;
  uint uIGPDomain;

  // Get node from context
  pNode= (SNetNode *) cli_context_get_item_at_top(pContext);
  // Add ospf domain id to node
  tokens_get_uint_at(pTokens, 1, &uIGPDomain);
  SIGPDomain * pIGPDomain = get_igp_domain((uint16_t)uIGPDomain); //creating and registering domain
  igp_domain_add_router(pIGPDomain, pNode);

  return CLI_SUCCESS;
}

// ----- cli_net_subnet_ospf_area ------------------------------------
/**
 * context: {subnet}
 * tokens: {area}
 */
int cli_net_subnet_ospf_area(SCliContext * pContext, STokens * pTokens)
{
  SNetSubnet * pSubnet;
  ospf_area_t uArea;

  // Get node from context
  pSubnet= (SNetSubnet *) cli_context_get_item_at_top(pContext);

  // set subnet ospf area
  tokens_get_uint_at(pTokens, 1, &uArea);
  if (subnet_set_OSPFArea(pSubnet, uArea) != 0) {
    LOG_SEVERE("Warning: correctness of ospf are info of links on subnet is not guaranteed!\n");
    return CLI_SUCCESS;
  }

  return CLI_SUCCESS;
}

// ----- cli_register_net_subnet_ospf --------------------------------------
/**
 *
 */
int cli_register_net_subnet_ospf(SCliCmds * pCmds)
{
  SCliCmds * pSubCmds= cli_cmds_create();
  SCliParams * pParams;

  pParams= cli_params_create();
  cli_params_add(pParams, "<area>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("area",
					cli_net_subnet_ospf_area,
					NULL, pParams));
  return cli_cmds_add(pCmds, cli_cmd_create("ospf", NULL,
					   pSubCmds, NULL));
}

// ----- cli_register_net_node_ospf --------------------------------------
/**
 *
 */
int cli_register_net_node_ospf(SCliCmds * pCmds)
{
  SCliCmds * pSubCmds= cli_cmds_create();
  SCliParams * pParams;

  pParams= cli_params_create();
  cli_params_add(pParams, "<igp-domain-id>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("domain",
					cli_net_node_ospf_domain,
					NULL, pParams));
  
  pParams= cli_params_create();
  cli_params_add(pParams, "<area>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("area",
					cli_net_node_ospf_area,
					NULL, pParams));
  return cli_cmds_add(pCmds, cli_cmd_create("ospf", NULL,
					   pSubCmds, NULL));
}

// ----- cli_ctx_create_ospf_domain ----------------------------------
/**
 * Create a context for the given BGP domain.
 *
 * context: {}
 * tokens: {ospf-domain-id}
 */
int cli_ctx_create_ospf_domain(SCliContext * pContext, void ** ppItem)
{
  unsigned int uDomainNumber;

  /* Get BGP domain from the top of the context */
  if (tokens_get_uint_at(pContext->pTokens, 0, &uDomainNumber) != 0) {
    LOG_SEVERE("Error: invalid ospf domain id number \"%s\"\n",
	       tokens_get_string_at(pContext->pTokens, 0));
    return CLI_ERROR_CTX_CREATE;
  }

  /* Check that the IGP domain exists */
  if (!exists_igp_domain((uint16_t) uDomainNumber)) {
    LOG_SEVERE("Error: domain \"%u\" does not exist.\n", uDomainNumber);
    return CLI_ERROR_CTX_CREATE;
  }

  *ppItem = get_igp_domain(uDomainNumber);
  
  return CLI_SUCCESS;
}

// ----- cli_ctx_destroy_bgp_domain ---------------------------------
void cli_ctx_destroy_ospf_domain(void ** ppItem)
{
}

// ----- cli_net_node_link_ospf_area ------------------------------------
/**
 * context: {node, link} 
 * tokens: {node-address, destination-prefix, area}
 */
int cli_net_node_link_ospf_area(SCliContext * pContext, STokens * pTokens)
{
  SNetNode * pNode;
  ospf_area_t tArea;
  int iReturn = CLI_SUCCESS;
  SNetLink * pLink;
  SPrefix sDestPrefix;
  
  pLink = (SNetLink *) cli_context_get_item_at_top(pContext);
  pNode = (SNetNode *) cli_context_get_item_at(pContext, 0);
  
  if (tokens_get_uint_at(pTokens, 2, &tArea)) {
    LOG_SEVERE("Error: invalid value for ospf-area (%s)\n",
	       tokens_get_string_at(pTokens, 2));
    return CLI_ERROR_COMMAND_FAILED;
  }
  link_get_prefix(pLink, &sDestPrefix);
  
  switch(node_link_set_ospf_area(pNode, sDestPrefix, tArea)){
    case  OSPF_LINK_OK : 
    break;
    
    case  OSPF_LINK_TO_MYSELF_NOT_IN_AREA : 
            LOG_SEVERE("Warning: ospf area link is changed\n");
    break;
    
    case  OSPF_SOURCE_NODE_NOT_IN_AREA    : 
            LOG_SEVERE("Error: source node must belong to area\n");
	    iReturn = CLI_ERROR_COMMAND_FAILED;
    break;
    
    case  OSPF_DEST_NODE_NOT_IN_AREA      : 
            LOG_SEVERE("Error: destination node must belongs to area\n");
	    iReturn = CLI_ERROR_COMMAND_FAILED;
    break;
    
    case  OSPF_DEST_SUBNET_NOT_IN_AREA    : 
            LOG_SEVERE("Error: destination subnet must belongs to area\n");
	    iReturn = CLI_ERROR_COMMAND_FAILED;
    break;
    
    case  OSPF_SOURCE_NODE_LINK_MISSING   : //never verified because link is found in context
            LOG_SEVERE("Error: link is not present\n");
	    iReturn = CLI_ERROR_COMMAND_FAILED;
    break;
    default : LOG_SEVERE("Error: unknown reasons\n");
	      iReturn = CLI_ERROR_COMMAND_FAILED;
  }
  
  return iReturn;
}

// ----- cli_register_net_node_link_ospf --------------------------------
void cli_register_net_node_link_ospf(SCliCmds * pCmds)
{
  SCliCmds * pSubCmds = cli_cmds_create();
  SCliParams * pParams = cli_params_create();

  cli_params_add(pParams, "<ospf-area>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("area",
					cli_net_node_link_ospf_area,
					NULL, pParams));
  
  cli_cmds_add(pCmds, cli_cmd_create("ospf", NULL, pSubCmds, NULL));
}

// ----- cli_net_ospf_domain_show -----------------------------------
/**
 * Show routers belong to domain
 *
 * context: {domain}
 * tokens: {domain-id}
 */
int cli_net_ospf_domain_show(SCliContext * pContext, STokens * pTokens)
{
  SIGPDomain * pDomain = cli_context_get_item_at_top(pContext);
  igp_domain_dump(stdout, pDomain);
  return CLI_SUCCESS;
}


// ----- cli_net_ospf_domain_show -----------------------------------
/**
 * Show routers belong to domain
 *
 * context: {domain}
 * tokens: {domain-id}
 */
int cli_net_ospf_domain_compute_route(SCliContext * pContext, STokens * pTokens)
{
  SIGPDomain * pDomain = cli_context_get_item_at_top(pContext);
  if (ospf_domain_build_route(pDomain->uNumber) >= 0);
    return CLI_SUCCESS;
  return CLI_ERROR_COMMAND_FAILED;
}


int cli_register_net_ospf_domain_show(SCliCmds * pCmds){
    return cli_cmds_add(pCmds, cli_cmd_create("show",
					cli_net_ospf_domain_show,
					NULL, NULL));
}

int cli_register_net_ospf_domain_compute_route(SCliCmds * pCmds){
    return cli_cmds_add(pCmds, cli_cmd_create("compute-route",
					cli_net_ospf_domain_compute_route,
					NULL, NULL));
}

int cli_register_net_ospf_domain(SCliCmds * pCmds)
{
  SCliCmds * pSubCmds;
  SCliParams * pParams;

  pSubCmds= cli_cmds_create();
  cli_register_net_ospf_domain_show(pSubCmds);
  cli_register_net_ospf_domain_compute_route(pSubCmds);
  pParams=cli_params_create();
  cli_params_add(pParams, "<ospf-domain-id>", NULL);


  return   cli_cmds_add(pCmds, cli_cmd_create_ctx("domain",
						cli_ctx_create_ospf_domain,
						cli_ctx_destroy_ospf_domain,
						pSubCmds, pParams));
}


// ----- cli_register_net_ospf --------------------------------------
/**
 *
 */
int cli_register_net_ospf(SCliCmds * pCmds)
{

//   pSubCmds= cli_cmds_create();
//   pParams= cli_params_create();
//   cli_params_add(pParams, "<id>", NULL); 
//   cli_cmds_add(pSubCmds, cli_cmd_create("domain", cli_net_show_nodes,
// 					NULL, pParams));
  SCliCmds * pSubCmds;
//   SCliParams * pParams = cli_params_create();
  
  pSubCmds= cli_cmds_create();
  cli_register_net_ospf_domain(pSubCmds);

						
  return cli_cmds_add(pCmds, cli_cmd_create("ospf", NULL,
					    pSubCmds, NULL));
}
