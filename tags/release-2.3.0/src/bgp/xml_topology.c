// ==================================================================
// @(#)xml_topology.c
//
// @author Sebastien Tandel (standel@info.ucl.ac.be)
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 08/09/2003
// $Id: xml_topology.c,v 1.7 2009-03-24 15:56:22 bqu Exp $
// ==================================================================

//TODO :  1) spf-prefix has to be done !
//	  2) peer up has to be done !

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include <libgds/log.h>
#include <libgds/fifo.h>
#include <libgds/array.h>
#include <libgds/radix-tree.h>
#include <libgds/xml.h>

#include <bgp/xml_topology.h>
#include <bgp/filter.h>
#include <bgp/filter_t.h>
#include <bgp/as.h>
#include <bgp/as_t.h>
#include <bgp/peer.h>

#include <net/prefix.h>
#include <net/link.h>
#include <net/protocol.h>
//#include <net/domain.h>
#include <net/network.h>

#ifdef HAVE_XML

// ----- xml_parse_node_add ------------------------------------------
/**
 *
 *
 */
SNetNode * xml_parse_node_add(SNetwork * pNetwork, net_addr_t ipAddr, 
				uint32_t uAS, const char * pcName)
{
  SNetNode * pNode;
  //  SNetDomain * pDomain;

  pNode = node_create(ipAddr);
  assert(!network_add_node(pNetwork, pNode));

  //  pDomain = network_domain_get(pNetwork, uAS);
  node_set_as(pNode, pDomain);
  node_set_name(pNode, pcName);
  //  domain_node_add(pDomain, pNode);

  return pNode;
}

// ------ xml_parse_node_interface_add -------------------------------
/**
 *
 *
 */
void xml_parse_node_interface_add(SNetNode * pNode, net_addr_t tAddr, 
					SPrefix * tMask)
{
  SNetInterface * pInterface = node_interface_create();

  pInterface->tAddr = tAddr;
  pInterface->tMask = tMask;

  node_interface_add(pNode, pInterface);
}

// ----- xml_parse_retrieve_node_info ---------------------------------
/**
 *
 *
 */
void xml_parse_retrieve_node_info(xmlHandle * hXML, SNetwork * pNetwork, 
								  uint32_t uAS)
{
  char * cAttr, * sName = NULL;
  char * pcEndPtr;
  net_addr_t ipAddr;
  SPrefix * tMask = NULL;
  SNetNode * pNode = NULL;

  while (xml_nodeset_search(hXML, "node") == 0) {
    xml_nodeset_push_node(hXML);
    cAttr = (char *)xml_node_get_attribute(hXML, "id", XML_STRING_DATA_TYPE);

    if (xml_nodeset_search(hXML, "rid") == 0) {
      sName = cAttr;
      cAttr = (char *)xml_node_get_content(hXML, XML_STRING_DATA_TYPE);
    }
    if (ip_string_to_address(cAttr, &pcEndPtr, &ipAddr)) 
      LOG_DEBUG("xml_parse_retrieve_node_info> can't convert string to ip address : %s.\n", cAttr);
    LOG_EVERYTHING ("Node ID : %s\n", cAttr);
    xml_char_free(cAttr);

    xml_nodeset_pop_node(hXML);
    xml_nodeset_push_node(hXML);

    if (xml_nodeset_search(hXML, "name") == 0) {
	cAttr = (char *)xml_node_get_content(hXML, XML_STRING_DATA_TYPE);
	LOG_EVERYTHING("Node Name : %s\n", cAttr);
	pNode = xml_parse_node_add(pNetwork, ipAddr, uAS, cAttr);
	xml_char_free(cAttr);
	if (sName != NULL)
	  xml_char_free(sName);
    } else {
	LOG_EVERYTHING("No Node Name\n");
	if (sName != NULL) {
	  pNode = xml_parse_node_add(pNetwork, ipAddr, uAS, sName);
	  xml_char_free(sName);
	} else
	  pNode = xml_parse_node_add(pNetwork, ipAddr, uAS, "");
    }

    while (xml_nodeset_search(hXML, "interface") == 0) {
      cAttr = (char *) xml_node_get_attribute(hXML, "id", XML_STRING_DATA_TYPE);
      LOG_EVERYTHING ("\tID interface : %s\n", cAttr);
      xml_char_free(cAttr);

      xml_nodeset_push_node(hXML);
      if (xml_nodeset_search(hXML, "ip") == 0) {
	cAttr = (char *)xml_node_get_attribute(hXML, "mask", XML_STRING_DATA_TYPE);
	LOG_EVERYTHING("\tMASK interface : %s\n", cAttr);
	tMask = MALLOC(sizeof(SPrefix));
	if (ip_string_to_prefix(cAttr, &pcEndPtr, tMask)) 
	  LOG_DEBUG("xml_parse_retrieve_node_info> can't convert string to ip address : %s.\n", cAttr);
	xml_char_free(cAttr);

	cAttr = (char *)xml_node_get_content(hXML, XML_STRING_DATA_TYPE);
	LOG_EVERYTHING("\tIP interface : %s\n", cAttr);
	if (ip_string_to_address(cAttr, &pcEndPtr, &ipAddr)) 
	  LOG_DEBUG("xml_parse_retrieve_node_info> can't convert string to ip address : %s.\n", cAttr);
	xml_char_free(cAttr);
      }
      xml_nodeset_pop_node(hXML);

      xml_parse_node_interface_add(pNode, ipAddr, tMask);       

      xml_nodeset_get_next_node(hXML);
    }
    xml_nodeset_pop_node(hXML);
    xml_nodeset_get_next_node(hXML);
  }
}

struct LinkInfo {
  char *      cNodeId;
  SNetNode *  pNodeFrom;
  net_addr_t  tFromIf;
  SNetNode *  pNodeTo;
  net_addr_t  tToIf;
};
typedef struct LinkInfo SLinkInfo;
// ----- xml_parse_xml_parse_retrieve_link_info_dir ------------------
/**
 *
 *
 */
int xml_parse_xml_parse_retrieve_link_info_dir(xmlHandle * hXML, 
    SNetwork * pNetwork, char * nodeName, SLinkInfo * pLink)
{
  net_addr_t ipAddr, ipIf = 0;
  char * cAttr, * pcEndPtr;

  if (xml_nodeset_search(hXML, nodeName) == 0) {
    cAttr = (char *)xml_node_get_attribute(hXML, "node", XML_STRING_DATA_TYPE);
    if (ip_string_to_address(cAttr, &pcEndPtr, &ipAddr)) 
      LOG_DEBUG("xml_parse_retrieve_node_info> can't convert string to"
		  "ip address : %s.\n", cAttr);
    LOG_EVERYTHING("\tnode \"%s\" : %s\n", nodeName, cAttr);
    xml_char_free(cAttr);

    cAttr = (char *)xml_node_get_attribute(hXML, "if", XML_STRING_DATA_TYPE);
    if (cAttr == NULL) {
      cAttr = (char *)xml_node_get_attribute(hXML, "as", XML_STRING_DATA_TYPE);
      LOG_EVERYTHING("\tAS \"to\" : %s\n", cAttr);
    } else {
      ip_string_to_address(cAttr, &pcEndPtr, &ipIf);
      LOG_EVERYTHING("\tif \"%s\" : %s\n", nodeName, cAttr);
    }
    xml_char_free(cAttr);
  }
 
  if (!strcmp(nodeName, "from")) {
    pLink->pNodeFrom = network_find_node(pNetwork, ipAddr);
    pLink->tFromIf = ipIf;
  } else {
    pLink->pNodeTo = network_find_node(pNetwork, ipAddr);
    pLink->tToIf = ipIf;
  }
  return 0;
}



// ----- xml_parse_link_info_compare -------------------------------------------
/**
 *
 *
 */
int xml_parse_link_info_compare(void * pItem1, void * pItem2, unsigned int uEltSize)
{
  int iCmpResult;
  SLinkInfo * pLink1 = *((SLinkInfo **)pItem1);
  SLinkInfo * pLink2 = *((SLinkInfo **)pItem2);

  iCmpResult = strcmp(pLink1->cNodeId, pLink2->cNodeId);

  if (iCmpResult < 0)
    return -1;
  else if (iCmpResult > 0)
    return 1;
  else
    return 0;
}

// ----- xml_parse_link_info_destroy -------------------------------------------
/**
 *
 *
 */
void xml_parse_link_info_destroy(void * pItem)
{
  SLinkInfo * pLink = *((SLinkInfo **) pItem);

  FREE (pLink->cNodeId);
  FREE (pLink);
}

// ----- link_info_dump ----------------------------------------------
/**
 *
 *
 */
void link_info_dump(SLinkInfo * pLink)
{
  if (pLink) {
    LOG_DEBUG("cNodeId : %s\n", pLink->cNodeId);
    printf("\t"); (pLink->pNodeFrom) ? node_dump(pLink->pNodeFrom) : printf("no node\n");
    printf("\t"); (pLink->pNodeTo) ? node_dump(pLink->pNodeTo) : printf("no node\n");
  } else
    LOG_DEBUG("no pLink\n");
}

// ----- link_info_push ----------------------------------------------
/**
 *
 *
 */
void xml_parse_link_info_add(SPtrArray * aNodes, char * pcNodeId, SLinkInfo * pLink)
{
  pLink->cNodeId = pcNodeId;

  ptr_array_add(aNodes, &pLink);
}


// ----- xml_parse_link_info_search --------------------------------------------
/**
 *
 *
 */
SLinkInfo * xml_parse_link_info_search(SPtrArray * aNodes, char * pcNodeId)
{
  unsigned int uIndex;
  SLinkInfo * pLinkTmp = MALLOC(sizeof(SLinkInfo));
  pLinkTmp->cNodeId = pcNodeId;
  
  if (ptr_array_sorted_find_index(aNodes, &pLinkTmp, &uIndex)) {
    LOG_WARNING("xml_parse_link_info_search> Element : %s not found.\n", pcNodeId);
    return NULL;
  }
  FREE(pLinkTmp);

  return (SLinkInfo *)(aNodes->data[uIndex]);
}

// ----- xml_parse_retrieve_link_info ------------------------------------------
/**
 *
 *
 */
void xml_parse_retrieve_link_info(xmlHandle * hXML, SNetwork * pNetwork, 
    SPtrArray * aNodes)
{
  char * cAttr, * NodeId;
  SLinkInfo * pLink;
  

  while (xml_nodeset_search(hXML, "link") == 0) {
    xml_nodeset_push_node(hXML);
    cAttr = (char *)xml_node_get_attribute(hXML, "id", XML_STRING_DATA_TYPE);
    NodeId = MALLOC(strlen(cAttr)+1);
    strcpy(NodeId, cAttr);
    LOG_EVERYTHING("Link ID : %s\n", cAttr);
    xml_char_free(cAttr);

    pLink = MALLOC(sizeof(SLinkInfo));
    xml_parse_xml_parse_retrieve_link_info_dir(hXML, pNetwork, "from", pLink);
    if (!pLink->pNodeFrom)
      LOG_WARNING("xml_parse_retrieve_link_info> no node\n");
    xml_nodeset_pop_node(hXML);

    xml_nodeset_push_node(hXML);
    xml_parse_xml_parse_retrieve_link_info_dir(hXML, pNetwork, "to", pLink);
    if (!pLink->pNodeTo)
      LOG_WARNING("xml_parse_retrieve_link_info> no node\n");
    xml_nodeset_pop_node(hXML);

    xml_parse_link_info_add(aNodes, NodeId, pLink);

    xml_nodeset_get_next_node(hXML);
  }
  xml_nodeset_search_reset(hXML);
}

// ----- xml_parse_node_link_add -------------------------------------
/**
 * TODO : choose the link with the lowest delay in case of backup link
 * betweeen two same nodes!
 *
 */
void xml_parse_igp_link_add(SLinkInfo * pLink, uint32_t uDelay)
{
  if (pLink->pNodeFrom && pLink->pNodeTo) {
    LOG_DEBUG("%ul %ul\n", pLink->pNodeFrom->tAddr, pLink->tFromIf);
   // if (pLink->pNodeFrom->tAddr == pLink->tFromIf) {
      assert(node_add_link(pLink->pNodeFrom, pLink->pNodeTo, uDelay, 0) >= 0);
    /*}
    assert(!node_interface_link_add(pLink->pNodeFrom, pLink->tFromIf, pLink->tToIf, 
			      uDelay));*/
  } else
    LOG_WARNING("xml_parse_igp_link_add> WARNING : There is at least " 
	"one node inexistant for the following link : %s\n", pLink->cNodeId);
}

// ----- xml_parse_retrieve_igp_info ---------------------------------
/**
 *
 *
 */
void xml_parse_retrieve_igp_info(xmlHandle * hXML, SPtrArray * aNodes)
{
  char * cAttr;
  int iAttr = 0, iCpt = 0;
  SLinkInfo * pLink = NULL;
  
  if (xml_nodeset_search(hXML, "igp") == 0) {
    while (xml_nodeset_search(hXML, "link") == 0 && iCpt < 1) {
      xml_nodeset_push_node(hXML);
      cAttr = (char *)xml_node_get_attribute(hXML, "id", XML_STRING_DATA_TYPE);
      pLink = xml_parse_link_info_search(aNodes, cAttr);
      LOG_EVERYTHING("Link ID igp : %s\n", cAttr);
      xml_char_free(cAttr);
      if (xml_nodeset_search(hXML, "metric") == 0) {
	cAttr = (char *)xml_node_get_content(hXML, XML_STRING_DATA_TYPE);
	iAttr = atoi(cAttr);
	xml_char_free(cAttr);
	LOG_EVERYTHING("\tMetric : %d\n", iAttr);
      }

/*      if (pLink->pNodeFrom && pLink->pNodeTo)
	node_add_link(pLink->pNodeFrom, pLink->pNodeTo, iAttr, 0);
      else
	LOG_WARNING("xml_parse_retrieve_igp_info> WARNING : There is at least " 
	    "one node inexistant for the following link : %s\n", pLink->cNodeId);*/
      xml_parse_igp_link_add(pLink, iAttr);

      xml_nodeset_pop_node(hXML);
      xml_nodeset_get_next_node(hXML);
    }
  }
}

// ----- xml_parse_bgp_router_add ---------------------------------------------------
/**
 *
 *
 */
SBGPRouter * xml_parse_bgp_router_add(SNetwork * pNetwork, char * pcName,
				      char * pcAddr, int ASID)
{
  char * pcEndPtr;
  SBGPRouter * pRouter;
  net_addr_t ipAddr;
  SNetNode * pNode;

  if (ip_string_to_address(pcAddr, &pcEndPtr, &ipAddr))
    LOG_DEBUG("xml_parse_retrieve_node_info> can't convert string to ip address : %s.\n", pcAddr);

  pNode = network_find_node(pNetwork, ipAddr);
  pRouter= bgp_router_create (ASID, pNode, 0);
  as_add_name(pRouter, strdup(pcName));
  assert(!node_register_protocol(pNode, NET_PROTOCOL_BGP, pRouter, 
			  (FNetNodeHandlerDestroy) bgp_router_destroy, 
			  as_handle_message));
  return pRouter;
}

// ----- xml_parse_retrieve_bgp_router_id -------------------------------------------
/**
 *
 *
 */
SBGPRouter * xml_parse_retrieve_bgp_router_id(xmlHandle * hXML, SNetwork * pNetwork, int iASID)
{
  char * cAttr, * cId;
  SBGPRouter * pRouter = NULL;

  cId = (char *)xml_node_get_attribute(hXML, "id", XML_STRING_DATA_TYPE);
  if (xml_nodeset_search(hXML, "router-id") == 0) {
    cAttr = (char *)xml_node_get_content(hXML, XML_STRING_DATA_TYPE);
    if ((pRouter = xml_parse_bgp_router_add(pNetwork, cId, cAttr, iASID)) == NULL)
      LOG_WARNING("xml_parse_retrieve_bgp_router_id> router %s can't be created.\n", cAttr);
    LOG_EVERYTHING("Router : %s\n", cAttr);
    xml_char_free(cAttr);
  }
  xml_char_free(cId);
  return pRouter;
}

// ----- xml_parse_bgp_network_add ---------------------------------------------
/**
 *
 *
 */
void xml_parse_bgp_network_add(SBGPRouter * pRouter, char * pcAddr)
{
  char * pcEndPtr;
  SPrefix Prefix;

  if (ip_string_to_prefix(pcAddr, &pcEndPtr, &Prefix)) {
    LOG_DEBUG("xml_parse_bgp_network_add> can't convert string to ip address : %s\n", pcAddr);
  }

  as_add_network(pRouter, Prefix);
}

// ----- retrieve_bgp_router_netowrks --------------------------------
/**
 *
 *
 */
void xml_parse_retrieve_bgp_networks(xmlHandle * hXML, SBGPRouter * pRouter)
{
  char * cAttr;

  if (xml_nodeset_search(hXML, "networks") == 0) {
    while (xml_nodeset_search(hXML, "network") == 0) {
      cAttr = (char *)xml_node_get_content(hXML, XML_STRING_DATA_TYPE);
      LOG_EVERYTHING("\tNetwork : %s\n", cAttr);
      xml_parse_bgp_network_add(pRouter, cAttr);
      xml_char_free(cAttr);
      xml_nodeset_get_next_node(hXML);
    }
  } 
}

// ----- xml_parse_retrieve_bgp_rules --------------------------------
/**
 *
 *
 */
SFilterMatcher * xml_parse_retrieve_bgp_rules_match(xmlHandle * hXML)
{
  char * cAttr, * pcEndPtr;
  SPrefix Prefix;
  SFilterMatcher * pMatcher = NULL;

  xml_nodeset_push_node(hXML);
  while (xml_nodeset_search(hXML, "match") == 0) {
    cAttr = (char *)xml_node_get_attribute(hXML, "prefix", XML_STRING_DATA_TYPE);
    LOG_EVERYTHING("\t\t\tmatch : %s\n", cAttr);
    if (ip_string_to_prefix(cAttr, &pcEndPtr, &Prefix)) 
      LOG_DEBUG("xml_parse_retrieve_bgp_rules_match> can't convert string to prefix : %s\n", cAttr);
    pMatcher = filter_match_prefix_equals(Prefix);
    xml_char_free(cAttr);

    xml_nodeset_get_next_node(hXML);
  }
  xml_nodeset_pop_node(hXML);
  return pMatcher;
    
}

// ----- xml_parse_bgp_rules_action --------------------------------------------
/**
 *
 *
 */
SFilterAction * xml_parse_bgp_rules_action(char * cType, char * cValue)
{
  SFilterAction * pAction = NULL;
  comm_t Community = 0;
  uint32_t uLocalPref = 0, uMetric = 0;
  
  if (!strcmp(cType, "add-community")) {
    Community = (comm_t)atoi(cValue);
    pAction = filter_action_comm_append(Community);
  }

  if (!strcmp(cType, "set-localpref")) {
    uLocalPref = (uint32_t)atoi(cValue);
    pAction = filter_action_pref_set(uLocalPref);
  }
  
  if (!strcmp(cType, "accept")) {
    pAction = filter_action_accept();
  }
  
  if (!strcmp(cType, "deny")) {
    pAction = filter_action_deny();
  }
  
  if (!strcmp(cType, "set-metric")) {
    uMetric = (uint32_t)atoi(cValue);
    pAction = filter_action_metric_set(uMetric);
  }

  if (pAction == NULL)
    LOG_WARNING("xml_parse_bgp_rules_action>Action Type is not already defined : %s\n", cType);

  return pAction;
}

// ----- xml_parse_retrieve_bgp_rules ------------------------------------------
/**
 *
 *
 */
SFilter * xml_parse_retrieve_bgp_rules(xmlHandle * hXML)
{
  char * cType, * cValue;
  SFilterMatcher * pMatcher = NULL;
  SFilterAction * pAction = NULL;
  SFilter * pFilter;
  char isOneRule = 0;

  
  pFilter = filter_create();

  while (xml_nodeset_search(hXML, "rule") == 0)  {
    LOG_EVERYTHING("\t\t\trule :\n");
    
    pMatcher = xml_parse_retrieve_bgp_rules_match(hXML); 

    xml_nodeset_push_node(hXML);
    while (xml_nodeset_search(hXML, "action") == 0) {
      cType = (char *)xml_node_get_attribute(hXML, "type", XML_STRING_DATA_TYPE);
      LOG_EVERYTHING("\t\t\t\taction : %s", cType);
      cValue = (char *)xml_node_get_attribute(hXML, "value", XML_STRING_DATA_TYPE);

      pAction = xml_parse_bgp_rules_action(cType, cValue);

      filter_add_rule(pFilter, pMatcher, pAction);

      xml_char_free(cType);
      if (cValue != NULL) {
	LOG_EVERYTHING("\tvalue : %s\n", cValue);
	xml_char_free(cValue);
      } else
	LOG_EVERYTHING("\n");
      xml_nodeset_get_next_node(hXML);
    }
    xml_nodeset_pop_node(hXML);
  
    xml_nodeset_get_next_node(hXML);
    isOneRule++;
  }

  if (!isOneRule) {
    filter_destroy(&pFilter);
    pFilter = NULL;
  }

  return pFilter;
}

// ----- xml_parse_bgp_filter_add ------------------------------------
/**
 *
 *
 */
void xml_parse_bgp_filter_add(SPeer * pPeer, SFilter * pInFilter, 
				SFilter * pOutFilter)
{
  if (pPeer) {
    if (pInFilter)
      peer_set_in_filter(pPeer, pInFilter);

    if (pOutFilter)
      peer_set_out_filter(pPeer, pOutFilter);
  }
}

// ---- xml_parse_retrieve_bgp_filters -------------------------------
/**
 * retrieve all the information about the filters (in, out)
 *
 */
void xml_parse_retrieve_bgp_filters(xmlHandle * hXML, SPeer * pPeer)
{
  SFilter * pInFilter = NULL, * pOutFilter = NULL;

  if (xml_nodeset_search_conditional_stop(hXML, "filters", "neighbor") == 0) {
    
    xml_nodeset_push_node(hXML);
    if (xml_nodeset_search(hXML, "in-filter") == 0) {
      LOG_EVERYTHING("\t\tin-filter(s)\n");
      pInFilter = xml_parse_retrieve_bgp_rules(hXML);
    }
    xml_nodeset_pop_node(hXML);
    
    xml_nodeset_push_node(hXML);
    if (xml_nodeset_search(hXML, "out-filter") == 0) {
      LOG_EVERYTHING("\t\tout-filter(s)\n");
      pOutFilter = xml_parse_retrieve_bgp_rules(hXML);
    }
    xml_nodeset_pop_node(hXML);
    xml_parse_bgp_filter_add(pPeer, pInFilter, pOutFilter);
  }
}

// ----- xml_parse_bgp_peer_add --------------------------------------
/**
 *
 *
 */
SPeer * xml_parse_bgp_peer_add(SBGPRouter * pRouter, const char * cAS, char * cIP)
{
  char * pcEndPtr;
  net_addr_t ipAddr;
  unsigned int uAS = atoi(cAS);

  if (ip_string_to_address(cIP, &pcEndPtr, &ipAddr)) 
    LOG_DEBUG("xml_parse_bgp_peer_add>can't convert string to ip address : %s\n", cIP);

  if (as_add_peer(pRouter, uAS, ipAddr, 0)) 
    LOG_WARNING("xml_parse_bgp_peer_add> peer already exists.\n");
  
  return as_find_peer(pRouter, ipAddr);
}

// ---- xml_parse_retrieve_bgp_neighbors --------------------------------
/**
 *
 *
 */
void xml_parse_retrieve_bgp_neighbors(xmlHandle * hXML, SBGPRouter * pRouter)
{
  char * cIP, * cAS;
  SPeer * pPeer;

  if (xml_nodeset_search(hXML, "neighbors") == 0) {
    while (xml_nodeset_search(hXML, "neighbor") == 0) {
      LOG_EVERYTHING("\tneighbor\n");
      cIP = (char *)xml_node_get_attribute(hXML, "ip", XML_STRING_DATA_TYPE);
      LOG_EVERYTHING("\t\tip : %s\n", cIP);
      cAS = (char *)xml_node_get_attribute(hXML, "as", XML_STRING_DATA_TYPE);
      LOG_EVERYTHING("\t\tas : %s\n", cAS);

      pPeer = xml_parse_bgp_peer_add(pRouter, cAS, cIP); 
      xml_char_free(cIP);
      xml_char_free(cAS);

      xml_nodeset_push_node(hXML);
      xml_parse_retrieve_bgp_filters(hXML, pPeer);
      xml_nodeset_pop_node(hXML);

      xml_nodeset_get_next_node(hXML);
    }
  } else 
    LOG_EVERYTHING("\tNo neighbor\n");
}

// ----- xml_parse_retrieve_bgp_info -------------------------------------------
/**
 * main function to retrieve all the information about the bgp routers
 *
 */
void xml_parse_retrieve_bgp_info(xmlHandle * hXML, SNetwork * pNetwork, int ASID)
{
  SBGPRouter * pRouter;

  while (xml_nodeset_search(hXML, "router") == 0) {
    xml_nodeset_push_node(hXML);
    pRouter = xml_parse_retrieve_bgp_router_id(hXML, pNetwork, ASID);
    xml_nodeset_pop_node(hXML);

    xml_nodeset_push_node(hXML);
    xml_parse_retrieve_bgp_networks(hXML, pRouter);
    xml_nodeset_pop_node(hXML);

    xml_nodeset_push_node(hXML);
    xml_parse_retrieve_bgp_neighbors(hXML, pRouter);
    xml_nodeset_pop_node(hXML);

    xml_nodeset_get_next_node(hXML);
  }
}

// ----- xml_topology_init -------------------------------------------
/**
 *
 *
 */
xmlHandle * xml_topology_init(char * cFile)
{
  xmlHandle * hXML;

//  aNode = fifo_create(500, NULL);

  if ((hXML = xml_parsing_init(cFile)) == NULL) {
    LOG_DEBUG("%s is not a valid XML file to load.\n", cFile);
    return NULL;
  }

  xml_xpath_search_init(hXML);

  return hXML;
}

// ----- xml_parse_retrieve_AS_id --------------------------------------------
/**
 *
 * 
 */
int  xml_parse_retrieve_AS_id (xmlHandle * hXML)
{
  int ASID;

  ASID = (int)xml_node_get_attribute(hXML, "ASID", XML_INT_DATA_TYPE);
  return ASID;
}

// ----- xml_parse_retrieve_AS_name --------------------------------------------
/**
 *
 *
 */
char * xml_parse_retrieve_AS_name(xmlHandle * hXML)
{
  return (char *)xml_node_get_attribute(hXML, "name", XML_STRING_DATA_TYPE);
}

// ----- xml_toppology_finalize --------------------------------------
/**
 *
 *
 */
void xml_topology_finalize(xmlHandle * hXML)
{
  //fifo_destroy(&aNode);
  xml_xpath_search_finalize(hXML);
  xml_parsing_finalize(hXML);
}

// ----- xml_topology_parse ------------------------------------------
/**
 *
 *
 */
int xml_topology_parse(xmlHandle * hXML, SNetwork * pNetwork)
{
  int iASID;
  char * pcASName;
  SPtrArray * aNodes;

  if (xml_xpath_search(hXML, "//domain")) {
    LOG_DEBUG("No valid search\n");
    return -1;
  }
  xml_nodeset_push_node(hXML);

  
  //get the info about the nodes to create 
  do {
    iASID = xml_parse_retrieve_AS_id(hXML);
    pcASName = xml_parse_retrieve_AS_name(hXML);
    network_domain_add(pNetwork, iASID, pcASName);
    xml_char_free(pcASName);
    xml_parse_retrieve_node_info(hXML, pNetwork, iASID);
  } while (xml_nodeset_get_next(hXML) == 0);

  // We have to create the array here 'cause we have to insert *all* 
  // the nodes in the array before creating the link with their delay 
  // just after
  aNodes = ptr_array_create(ARRAY_OPTION_SORTED|ARRAY_OPTION_UNIQUE, 
			      xml_parse_link_info_compare, 
			      xml_parse_link_info_destroy);
  xml_xpath_first_nodeset(hXML);
  xml_nodeset_pop_node(hXML);
  xml_nodeset_push_node(hXML);
  //get the info about the link and their delay between the nodes created
  do {
    xml_nodeset_search_reset(hXML);
    xml_parse_retrieve_link_info(hXML, pNetwork, aNodes);
  } while (xml_nodeset_get_next(hXML) == 0);
  xml_xpath_first_nodeset(hXML);
  xml_nodeset_pop_node(hXML);
  xml_nodeset_push_node(hXML);
  do {
    xml_nodeset_search_reset(hXML);
    xml_parse_retrieve_igp_info(hXML, aNodes);
  } while (xml_nodeset_get_next(hXML) == 0);
  ptr_array_destroy(&aNodes);
  
  xml_xpath_first_nodeset(hXML);
  xml_nodeset_pop_node(hXML);
  //get all the info about the bgp routers we have to create
  do {
    iASID = xml_parse_retrieve_AS_id(hXML);
    xml_nodeset_search_reset(hXML);
    xml_parse_retrieve_bgp_info(hXML, pNetwork, iASID);

  } while (xml_nodeset_get_next(hXML) == 0);

  return 0;
}

// ----- xml_topology_load -------------------------------------------
/**
 *
 *
 */
int xml_topology_load(char * cFile, SNetwork * pNetwork)
{
  xmlHandle * hXML;

  if ((hXML = xml_topology_init(cFile)) == NULL) 
    return -1;

  if (xml_topology_parse(hXML, pNetwork))
    return -1;

  xml_topology_finalize(hXML);
  return 0;
}


// *************************************** //
//	      WRITING XML FILE		   //
// *************************************** //
#define FIRST_LEVEL   "  "
#define SECOND_LEVEL  "    "
#define THIRD_LEVEL   "      "
#define FOURTH_LEVEL  "        "
#define FIFTH_LEVEL   "          "
#define SIXTH_LEVEL   "            "
#define SEVENTH_LEVEL "              "


#define XML_BEG	    "<?xml version=\"1.0\"?>\n"
#define DOMAIN_BEG  "<domain name=\"%s\" ASID=\"%d\" xmlns:xsi=\"http://www.w3.org/2001/Schema-Instance\">\n"
#define TOPO_BEG    FIRST_LEVEL	  "<topology>\n"
#define NODES_BEG   SECOND_LEVEL  "<nodes>\n"
#define NODE_BEG    THIRD_LEVEL	  "<node id=\"%s\">\n"
#define NAME	    FOURTH_LEVEL  "<name>%s</name>\n"
#define INTS_BEG    FOURTH_LEVEL  "<interfaces>\n"
#define INT_BEG	    FIFTH_LEVEL	  "<interface id=\"%s\">\n"
#define LINKS_BEG   THIRD_LEVEL	  "<links>\n"
#define IGP_BEG	    FIRST_LEVEL	  "<igp>\n"
#define IGP_T_END   SECOND_LEVEL  "<igp type=\"%s\">\n"
#define BGP_BEG	    FIRST_LEVEL	  "<bgp>\n"
#define ROUTER_BEG  SECOND_LEVEL  "<router id=\"%s\">\n"
#define NETS_BEG    SECOND_LEVEL  "<networks>\n"
#define NEIGHS_BEG  SECOND_LEVEL  "<neighbors>\n"
#define NEIGH_BEG   THIRD_LEVEL	  "<neighbor as=\"%d\" ip=\"%s\">\n"
#define FILTERS_BEG FOURTH_LEVEL  "<filters>\n"
#define IFILTER_BEG FIFTH_LEVEL	  "<in-filter>\n"
#define OFILTER_BEG FIFTH_LEVEL	  "<out-filter>\n"
#define RULE_BEG    SIXTH_LEVEL	  "<rule>\n"
#define LINK_BEG    FOURTH_LEVEL	  "<link id=\"%s\">\n"
#define LINK_INT_BEG FOURTH_LEVEL  "<link id=\"%s\" type=\"interdomain\">\n"
#define STATIC_BEG  FIFTH_LEVEL  "<static>\n"

#define RID	    FOURTH_LEVEL  "<rid>%s</rid>\n"
#define IP	    SIXTH_LEVEL	  "<ip mask=\"%s\">%s</ip>\n"
#define FROM	    FIFTH_LEVEL  "<from node=\"%s\" if=\"%s\" />\n"
#define TO_NODE	    FIFTH_LEVEL  "<to node=\"%s\" if=\"%s\" />\n"
#define TO_AS	    FIFTH_LEVEL  "<to as=\"%d\" if=\"%s\" />\n"
#define METRIC	    SIXTH_LEVEL  "<metric>%d</metric>\n"
#define ROUTERID    THIRD_LEVEL	  "<router-id>%s</router-id>\n"
#define NET	    FOURTH_LEVEL  "<network>%s</network>\n"
#define ACTION_TV   SEVENTH_LEVEL "<action type=\"%s\" value=\"%d\" />\n"
#define ACTION_T    SEVENTH_LEVEL "<action type=\"%s\" />\n"
#define MATCH	    SEVENTH_LEVEL "<match prefix=\"%s\" />\n"


#define INTS_END    FOURTH_LEVEL  "</interfaces>\n"
#define INT_END	    FIFTH_LEVEL	  "</interface>\n"
#define NODE_END    THIRD_LEVEL	  "</node>\n"
#define NODES_END   SECOND_LEVEL  "</nodes>\n"
#define LINK_END    FOURTH_LEVEL	  "</link>\n"
#define LINKS_END   THIRD_LEVEL  "</links>\n"
#define TOPO_END    FIRST_LEVEL	  "</topology>\n"
#define IGP_END	    FIRST_LEVEL	  "</igp>\n"
#define BGP_END	    FIRST_LEVEL	  "</bgp>\n"
#define NETS_END    SECOND_LEVEL  "</networks>\n"
#define NEIGHS_END  SECOND_LEVEL  "</neighbors>\n"
#define FILTERS_END FOURTH_LEVEL  "</filters>\n"
#define IFILTER_END FIFTH_LEVEL	  "</in-filter>\n"
#define OFILTER_END FIFTH_LEVEL	  "</out-filter>\n"
#define RULE_END    SIXTH_LEVEL	  "</rule>\n"
#define NEIGH_END   THIRD_LEVEL	  "</neighbor>\n"
#define ROUTER_END  SECOND_LEVEL  "</router>\n"
#define DOMAIN_END		  "</domain>\n"
#define STATIC_END  FIFTH_LEVEL  "</static>\n"

struct NodesAS {
  uint16_t uAS;
  SPtrArray * aNodes;
};
typedef struct NodesAS SNodesAS;

// ----- xml_write_init ----------------------------------------------
/**
 *
 *
 */
void xml_write_init(FILE * pFile)
{
  fprintf(pFile, XML_BEG);
}

// ----- xml_write_nodes ---------------------------------------------
/**
 *
 *
 */
int xml_write_nodes(SNodesAS * pNodesAS, FILE * pFile)
{
  int iIndexNode, iIndexInterface;
  SPtrArray * aNodes; 
  SNetNode * pNode;
  SNetInterface * pInterface;
  char * pcAddr, * tMask; 
  
  if (pNodesAS != NULL) {
    aNodes = pNodesAS->aNodes;

    pcAddr = MALLOC(16);
    fprintf(pFile, NODES_BEG);
      for (iIndexNode = 0; iIndexNode < ptr_array_length(aNodes); iIndexNode++) {
	pNode = aNodes->data[iIndexNode];
	
	ip_address_to_string(pcAddr, pNode->tAddr);

	fprintf(pFile, NODE_BEG, pNode->pcName);
	  fprintf(pFile, RID, pcAddr);
	  if (pNode->pcName)
	    fprintf(pFile, NAME, pNode->pcName);
	  else
	    fprintf(pFile, NAME, "");
	  fprintf(pFile, INTS_BEG);

	tMask = MALLOC(20);
	for (iIndexInterface = 0; iIndexInterface < ptr_array_length(pNode->aInterfaces); iIndexInterface++) {
	    pInterface = pNode->aInterfaces->data[iIndexInterface];
	    ip_address_to_string(pcAddr, pInterface->tAddr);
	    ip_prefix_to_string(tMask, pInterface->tMask);
	      
	      fprintf(pFile, INT_BEG, pcAddr);
		fprintf(pFile, IP, tMask, pcAddr);
	      fprintf(pFile, INT_END);
	}
	FREE(tMask);
	  fprintf(pFile, INTS_END);
	fprintf(pFile, NODE_END);
      }
    fprintf(pFile, NODES_END);
    FREE (pcAddr);
  } else
    LOG_DEBUG("pNodeAS == NULL\n");
  return 0;
}

// ----- xml_write_make_id -------------------------------------------
/**
 *
 *
 */
char * xml_write_make_id(char * pcAddr1, char * pcAddr2)
{
  char * pcId = MALLOC(31);

  sprintf(pcId, "%s_%s", pcAddr1, pcAddr2);

  return pcId;
}

#define WRITE_LINKS 0x00
#define WRITE_IGP   0x01
// ----- xml_write_links_igp -----------------------------------------
/**
 *
 *
 */
void xml_write_links_igp(SNodesAS * pNodesAS, FILE * pFile, 
					const char cWritingType)
{
  SPtrArray * aNodes = pNodesAS->aNodes;
  unsigned int uIndexNodes, uIndexLinks;
  SNetLink * pLink = NULL;
  SNetNode * pNode = NULL;
  char * pcAddr1, * pcAddr2, * pcId;
  uint32_t uAS1, uAS2;

  if (cWritingType != WRITE_LINKS)
    fprintf(pFile, IGP_BEG);

    fprintf(pFile, LINKS_BEG);
  
    for (uIndexNodes = 0; uIndexNodes < ptr_array_length(aNodes); 
	  uIndexNodes++) {
      pNode = aNodes->data[uIndexNodes];
      pcAddr1 = MALLOC(16);
      ip_address_to_string(pcAddr1, pNode->tAddr);
      uAS1 = node_get_as_id(pNode);

      for (uIndexLinks = 0; uIndexLinks < ptr_array_length(pNode->pLinks); 
	    uIndexLinks++) {
	pLink = (SNetLink *)pNode->pLinks->data[uIndexLinks];

	pcAddr2 = MALLOC(16);
	ip_address_to_string(pcAddr2, pLink->tAddr);
	pcId = xml_write_make_id(pcAddr1, pcAddr2);

	uAS2 = node_get_as_id(network_find_node(network_get(), 
						pLink->tAddr));

	if (cWritingType == WRITE_LINKS) {
	  if (uAS1 == uAS2)
	    fprintf(pFile, LINK_BEG, pcId);
	  else
	    fprintf(pFile, LINK_INT_BEG, pcId);
	    fprintf(pFile, FROM, pcAddr1, pcAddr1);
	  if (uAS1 == uAS2)
	    fprintf(pFile, TO_NODE, pcAddr2, pcAddr2);
	  else
	    fprintf(pFile, TO_AS, uAS2, pcAddr2);
	  fprintf(pFile, LINK_END);
	} else { 
	  fprintf(pFile, LINK_BEG, pcId);
	    fprintf(pFile, STATIC_BEG);
	      fprintf(pFile, METRIC, link_get_igp_weight(pLink));
	    fprintf(pFile, STATIC_END);
	  fprintf(pFile, LINK_END);
	}
	FREE(pcAddr2);
	FREE(pcId);
      }	
      FREE(pcAddr1);
    }
  fprintf(pFile, LINKS_END);
  if (cWritingType != WRITE_LINKS)
    fprintf(pFile, IGP_END);
}

// ----- xml_write_bgp_peer_filter_matcher ---------------------------
/**
 *
 *
 */
char * xml_write_bgp_peer_filter_matcher(SFilterMatcher * pMatcher)
{
  char * pcMatcher = NULL;
  if (pMatcher != NULL) {
    switch(pMatcher->uCode) {
      case FT_MATCH_OP_AND:
      case FT_MATCH_OP_OR:
      case FT_MATCH_OP_NOT:
      case FT_MATCH_COMM_CONTAINS:
      case FT_MATCH_PREFIX_IN:
	LOG_WARNING("xml_write_bgp_peer_filter_matcher>Matcher not already"
		      "implemented in xml dump.\n");
	break;
      case FT_MATCH_PREFIX_EQUALS:
	pcMatcher = MALLOC(20);
	ip_prefix_to_string(pcMatcher, ((SPrefix *) pMatcher->auParams));
	break;
      default:
	LOG_WARNING("xml_Write_bgp_peer_filter_matcher>Matcher is not valid\n");
    }
  }
  return pcMatcher;
}

typedef struct {
  char * pcType;
  uint32_t uValue;
} SAction;
// ----- xml_Write_bgp_peer_filter_action ----------------------------
/**
 *
 *
 */
SAction * xml_write_bgp_peer_filter_action(SFilterAction * pAction)
{
  SAction * psAction = MALLOC(sizeof(SAction));;
  psAction->pcType = NULL;
  psAction->uValue = 0;

  if (pAction) {
    switch (pAction->uCode) {
      case FT_ACTION_ACCEPT:
	psAction->pcType = MALLOC(7);
	strcpy(psAction->pcType, "accept");
	break;
      case FT_ACTION_DENY:
	psAction->pcType = MALLOC(5);
	strcpy(psAction->pcType, "deny");
	break;
      case FT_ACTION_COMM_APPEND:
	psAction->pcType = MALLOC(14);
	strcpy(psAction->pcType, "add-community");
	psAction->uValue = *((uint32_t *) pAction->auParams);
	break;
      case FT_ACTION_PREF_SET:
	psAction->pcType = MALLOC(14);
	strcpy(psAction->pcType, "set-localpref");
	psAction->uValue = *((uint32_t *) pAction->auParams);
	break;
      case FT_ACTION_METRIC_SET:
	psAction->pcType = MALLOC(11);
	strcpy(psAction->pcType, "set-metric");
	psAction->uValue = *((uint32_t *) pAction->auParams);
	break;
      case FT_ACTION_METRIC_INTERNAL:
      case FT_ACTION_ECOMM_APPEND:
      case FT_ACTION_PATH_PREPEND:
      case FT_ACTION_COMM_STRIP:
      case FT_ACTION_COMM_REMOVE:
	LOG_WARNING("xml_write_bgp_peer_filter_action>Action not already"
		  "implemented.\n");
	break;
      default:
	LOG_WARNING("xml_write_bgp_peer_filter_action>Action is not valid\n");
    }
  }
  return psAction;
}

// ----- xml_write_bgp_peer_filter -----------------------------------
/**
 *
 *
 */
void xml_write_bgp_peer_filter(FILE * pFile, SFilter * pFilter)
{
  unsigned int uIndex;
  char * pcMatcher;
  SAction * pAction;
  SFilterRule * pRule;

  if (pFilter) {
    for (uIndex = 0; uIndex < pFilter->pSeqRules->iSize; uIndex++) {
      fprintf(pFile, RULE_BEG);
      pRule = pFilter->pSeqRules->ppItems[uIndex];
      
      pcMatcher = xml_write_bgp_peer_filter_matcher(pRule->pMatcher);
      if (pcMatcher) {
        fprintf(pFile, MATCH, pcMatcher);
	FREE(pcMatcher);
      }

      pAction = xml_write_bgp_peer_filter_action(pRule->pAction);
      if (pAction->pcType) {
	if (pAction->uValue != 0)
	  fprintf(pFile, ACTION_TV, pAction->pcType, pAction->uValue);
	else
	  fprintf(pFile, ACTION_T, pAction->pcType);
      }
      FREE(pAction);
      fprintf(pFile, RULE_END);
    }
  }
}

// ----- xml_write_bgp_peer_filters ----------------------------------
/**
 *
 *
 */
void xml_write_bgp_peer_filters (FILE * pFile, SPeer * pPeer)
{
  fprintf(pFile, FILTERS_BEG);
  if (pPeer->pInFilter != NULL) {
    fprintf(pFile, IFILTER_BEG);
      xml_write_bgp_peer_filter(pFile, pPeer->pInFilter);
    fprintf(pFile, IFILTER_END);
  }

  if (pPeer->pOutFilter != NULL) {
    fprintf(pFile, OFILTER_BEG);
      xml_write_bgp_peer_filter(pFile, pPeer->pOutFilter);
    fprintf(pFile, OFILTER_END);
  }
  fprintf(pFile, FILTERS_END);
  
}

// ----- xml_write_bgp_neighbor --------------------------------------
/**
 *
 *
 */
void xml_write_bgp_neighbor(FILE * pFile, SBGPRouter * pRouter)
{
  uint32_t uIndex;
  SPeer * pPeer = NULL;
  char * pcAddr;
  
  fprintf(pFile, NEIGHS_BEG); 
    for (uIndex = 0; uIndex < ptr_array_length(pRouter->pPeers); 
	  uIndex++) {
      pPeer = pRouter->pPeers->data[uIndex];

      pcAddr = MALLOC(16);
      ip_address_to_string(pcAddr, pPeer->tAddr);

      fprintf(pFile, NEIGH_BEG, pPeer->uRemoteAS, pcAddr);
	xml_write_bgp_peer_filters(pFile, pPeer);
      fprintf(pFile, NEIGH_END);
      FREE(pcAddr);
    }
  fprintf(pFile, NEIGHS_END);
  
}

// ----- xml_write_bgp_networks --------------------------------------
/**
 *
 *
 */
void xml_write_bgp_networks(FILE * pFile, SBGPRouter * pRouter)
{
  uint32_t uIndex;
  SPrefix * pPrefix = NULL;
  char * pcPrefix;

  fprintf(pFile, NETS_BEG);
  for (uIndex = 0; uIndex < ptr_array_length(pRouter->pLocalNetworks);
	uIndex++) {
    pPrefix = pRouter->pLocalNetworks->data[uIndex];

    pcPrefix = MALLOC(20);
    ip_prefix_to_string(pcPrefix, pPrefix);

    fprintf(pFile, NET, pcPrefix);

    FREE(pcPrefix);

  }
  fprintf(pFile, NETS_END);
}

// ----- xml_write_bgp -----------------------------------------------
/**
 *
 *
 */
void xml_write_bgp(SNodesAS * pNodesAS, FILE * pFile)
{
  SPtrArray * aNodes = pNodesAS->aNodes;
  unsigned int uIndexNodes;
  SBGPRouter * pRouter = NULL;
  SNetNode * pNode = NULL;
  SNetProtocol * pProtocol;
  char * pcAddr = NULL;

  fprintf(pFile, SECOND_LEVEL BGP_BEG);
    for (uIndexNodes = 0; uIndexNodes < ptr_array_length(aNodes); 
	  uIndexNodes++) {
      pNode = aNodes->data[uIndexNodes];
      if ((pProtocol = protocols_get(pNode->pProtocols, 
				      NET_PROTOCOL_BGP)) != NULL) {
	pcAddr = MALLOC(16);
	ip_address_to_string(pcAddr, pNode->tAddr);
	pRouter = (SBGPRouter *)pProtocol->pHandler;
	fprintf(pFile, ROUTER_BEG, pRouter->pcName);
	  fprintf(pFile, ROUTERID, pcAddr);
	  xml_write_bgp_networks(pFile, pRouter);
	  xml_write_bgp_neighbor(pFile, pRouter);
	fprintf(pFile, ROUTER_END);
      }
    }
  fprintf(pFile, BGP_END);
}


// ------ nodes_by_as_compare ----------------------------------------
/**
 *
 *
 */
int nodes_by_as_compare(void * pItem1, void * pItem2, 
			  unsigned int uItemSize)
{
  SNodesAS * pNodesAS1 = *((SNodesAS **)pItem1);
  SNodesAS * pNodesAS2 = *((SNodesAS **)pItem2);

  if (pNodesAS1->uAS < pNodesAS2->uAS) 
    return -1;
  else if (pNodesAS1->uAS > pNodesAS2->uAS)
    return 1;
  else
    return 0;
}

// ----- xml_write_parse_each_node -----------------------------------
/**
 * We create an structure to sort each node by its AS number. 
 * It *had* to be done when we had no domain structure. We now can 
 * replace this by a walking into the pDomains array in SNetwork. But
 * we only can do this walkthrough if we loaded the topology with an 
 * xml file. 
 *
 * TODO:  1) walkthrough in pDomains 
 *	  2) change cBGP to made walkthrough possible when no xml
 *	  topology is loaded. (i.e. rexford, ...)
 *
 */
int xml_write_parse_each_node(uint32_t uKey, uint8_t uKeyLen, 
				void * pItem, void * pContext)
{
  SNetNode * pNode = (SNetNode *)pItem;
  SPtrArray * aNodesByAS = (SPtrArray *)pContext;
  SNodesAS * pNodesAS;
  unsigned int uIndex;
  uint16_t uAS = node_get_as_id(pNode);

  pNodesAS = MALLOC(sizeof(SNodesAS));
  pNodesAS->uAS = uAS;
  if (ptr_array_sorted_find_index(aNodesByAS, &pNodesAS, &uIndex)) {
    pNodesAS->aNodes = ptr_array_create(ARRAY_OPTION_UNIQUE, NULL, NULL);
    ptr_array_add(aNodesByAS, &pNodesAS);
  } else {
    FREE(pNodesAS);
    pNodesAS = (SNodesAS *)aNodesByAS->data[uIndex];
  }
  ptr_array_add(pNodesAS->aNodes, &pNode);

  return 0;
}

// ----- xml_write_parse_network -------------------------------------
/**
 *
 *
 */
SPtrArray * xml_write_parse_network (SNetwork * pNetwork)
{
  SPtrArray * aNodesByAS = ptr_array_create(ARRAY_OPTION_SORTED|
					    ARRAY_OPTION_UNIQUE,
					    nodes_by_as_compare, NULL);

  radix_tree_for_each(pNetwork->pNodes, xml_write_parse_each_node, 
			aNodesByAS);

  return aNodesByAS;
}

// ----- xml_write_domain_init ---------------------------------------
/**
 *
 *
 */
void xml_write_domain_init(SNodesAS * pNodesAS, FILE * pFile)
{
  SNetNode * pNode = pNodesAS->aNodes->data[0];
  SNetDomain * pDomain = pNode->pDomain;
  
  fprintf(pFile, DOMAIN_BEG, domain_get_name(pDomain), 
			      domain_get_as(pDomain));
}

// ----- xml_write_domain_finalize -----------------------------------
/**
 *
 *
 */
void xml_write_domain_finalize(FILE * pFile)
{
  fprintf(pFile, DOMAIN_END);
}

// ----- xml_topology_dump -------------------------------------------
/**
 *
 *
 */
int xml_topology_dump(SNetwork * pNetwork, char * pcFile)
{
  int iIndex;
  FILE * pFile; 
  SPtrArray * aNodesByAS;
  SNodesAS * pNodesAS;
  
  if ((pFile = fopen(pcFile, "w")) == NULL) {
    LOG_DEBUG("xml_topology_dump> can't open %s to write the "
		"topology.\n", pcFile);
    return -1;
  }

  aNodesByAS = xml_write_parse_network(pNetwork);
  
  xml_write_init(pFile);
  for (iIndex = 0; iIndex < ptr_array_length(aNodesByAS); iIndex++) {

    pNodesAS = (SNodesAS *)aNodesByAS->data[iIndex];
    
    xml_write_domain_init(pNodesAS, pFile);

    fprintf(pFile, TOPO_BEG);
    xml_write_nodes(pNodesAS, pFile);

    xml_write_links_igp(pNodesAS, pFile, WRITE_LINKS);
    fprintf(pFile, TOPO_END);

    xml_write_links_igp(pNodesAS, pFile, WRITE_IGP);

    xml_write_bgp(pNodesAS, pFile);

    xml_write_domain_finalize(pFile);
  }

  ptr_array_destroy(&aNodesByAS);
  fclose(pFile);
  return 0;
}

#endif
