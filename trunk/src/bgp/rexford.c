// ==================================================================
// @(#)rexford.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 28/07/2003
// @lastdate 27/01/2005
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <assert.h>
#include <string.h>

#include <libgds/tokenizer.h>

#include <bgp/as.h>
#include <bgp/as_t.h>
#include <bgp/filter.h>
#include <bgp/peer.h>
#include <bgp/rexford.h>
#include <net/network.h>
#include <net/protocol.h>

static SAS * AS[MAX_AS];
static SPtrArray * pASEdges= NULL;

// ----- rexford_get_as ---------------------------------------------
/**
 *
 */
SAS * rexford_get_as(uint16_t uASNum)
{
  return AS[uASNum];
}

// ----- rexford_load -----------------------------------------------
/**
 * Load the list of ASes and peerings. The format of the file is
 *   <AS1-number> <AS2-number> <peering-type>
 * where peering type can be
 *   0 for a peer-to-peer relationship
 *   1 for a provider (AS1) to customer (AS2) relationship
 */
int rexford_load(char * pcFileName, SNetwork * pNetwork)
{
  FILE * pFile;
  char acFileLine[80];
  unsigned int uAS1, uAS2, uRelation;
  int iError= 0;
  SNetNode * pNode1, * pNode2;
  SAS * pAS1, * pAS2;
  net_addr_t tAddr1, tAddr2;
  net_link_delay_t tDelay;
  int iIndex;
  STokenizer * pTokenizer;
  STokens * pTokens;
  uint32_t uLineNumber= 0;
  SNetProtocol * pProtocol;
  SPrefix sPrefix;

  for (iIndex= 0; iIndex < MAX_AS; iIndex++)
    AS[iIndex]= NULL;

  pTokenizer= tokenizer_create(" \t", 0, NULL, NULL);

  if ((pFile= fopen(pcFileName, "r")) != NULL) {
    while ((!feof(pFile)) && (!iError)) {
      if (fgets(acFileLine, sizeof(acFileLine), pFile) == NULL)
	break;
      uLineNumber++;

      if (tokenizer_run(pTokenizer, acFileLine) != TOKENIZER_SUCCESS) {
	LOG_SEVERE("Error: unexpected parse error in topology, line %u\n",
		   uLineNumber);
	iError= REXFORD_ERROR_UNEXPECTED;
	break;
      }
      
      pTokens= tokenizer_get_tokens(pTokenizer);

      // Set default value for optional parameters
      tDelay= 0;
      
      // Get and check mandatory parameters
      if (tokens_get_num(pTokens) < 3) {
	LOG_SEVERE("Error: not enouh parameters in topology, line %u\n",
		   uLineNumber);
	iError= REXFORD_ERROR_NUM_PARAMS;
	break;
      }
      if ((tokens_get_uint_at(pTokens, 0, &uAS1) != 0) ||
	  (tokens_get_uint_at(pTokens, 1, &uAS2) != 0) ||
	  (uAS1 >= MAX_AS) || (uAS2 >= MAX_AS)) {
	LOG_SEVERE("Error: invalid AS-NUM in topology, line %u\n",
		  uLineNumber);
	iError= REXFORD_ERROR_INVALID_ASNUM;
	break;
      }

      if ((tokens_get_uint_at(pTokens, 2, &uRelation) != 0) ||
	  ((uRelation != REXFORD_REL_PROV_CUST) &&
	   (uRelation != REXFORD_REL_PEER_PEER))) {
	LOG_SEVERE("Error: invalid relation in topology, line %u\n",
		  uLineNumber);
	iError= REXFORD_ERROR_INVALID_RELATION;
	break;
      }
      
      // Get optional parameters
      if (tokens_get_num(pTokens) > 3) {
	if (tokens_get_uint_at(pTokens, 3, &tDelay) != 0) {
	  LOG_SEVERE("Error: invalid delay in topology, line %u\n",
		    uLineNumber);
	  iError= REXFORD_ERROR_INVALID_DELAY;
	  break;
	}
      }

      // Limit number of parameters
      if (tokens_get_num(pTokens) > 4) {
	LOG_SEVERE("Error: too many arguments in topology, line %u\n",
		   uLineNumber);
	iError= 1;
	break;
      }
      
      // Create ASes if required
      tAddr1= (uAS1 << 16);
      tAddr2= (uAS2 << 16);

      // Find/create AS1's node
      pNode1= network_find_node(pNetwork, tAddr1);
      if (pNode1 == NULL) {
	pNode1= node_create(tAddr1);
	pAS1= as_create(uAS1, pNode1, 0);
	AS[uAS1]= pAS1;
	assert(!node_register_protocol(pNode1, NET_PROTOCOL_BGP, pAS1,
				       (FNetNodeHandlerDestroy) as_destroy,
				       as_handle_message));
	assert(!network_add_node(pNetwork, pNode1));
      } else {
	pProtocol= protocols_get(pNode1->pProtocols, NET_PROTOCOL_BGP);
	assert(pProtocol != NULL);
	pAS1= (SAS *) pProtocol->pHandler;
      }

      // Find/create AS2's node
      pNode2= network_find_node(pNetwork, tAddr2);
      if (pNode2 == NULL) {
	pNode2= node_create(tAddr2);
	pAS2= as_create(uAS2, pNode2, 0);
	AS[uAS2]= pAS2;
	assert(!node_register_protocol(pNode2, NET_PROTOCOL_BGP, pAS2,
				       (FNetNodeHandlerDestroy) as_destroy,
				       as_handle_message));
	assert(!network_add_node(pNetwork, pNode2));
      } else {
	pProtocol= protocols_get(pNode2->pProtocols, NET_PROTOCOL_BGP);
	assert(pProtocol != NULL);
	pAS2= (SAS *) pProtocol->pHandler;
      }

      // Add the link and check that this link did not already exist
      if (node_add_link(pNode1, pNode2, tDelay, 1) != 0) {
	LOG_SEVERE("Error: duplicate link (%u, %u) in topology, line %u\n",
		   uAS1, uAS2, uLineNumber);
	iError= REXFORD_ERROR_DUPLICATE_LINK;
	break;
      } else {
	// Add static routes
	sPrefix.tNetwork= tAddr2;
	sPrefix.uMaskLen= 32;
	assert(!node_rt_add_route(pNode1, sPrefix, tAddr2, tDelay,
				  NET_ROUTE_STATIC));
	sPrefix.tNetwork= tAddr1;
	sPrefix.uMaskLen= 32;
	assert(!node_rt_add_route(pNode2, sPrefix, tAddr1, tDelay,
				  NET_ROUTE_STATIC));
      }
      
      // Setup peering relations
      if (as_add_peer(pAS1, uAS2, tAddr2,
		      (uRelation == REXFORD_REL_PEER_PEER)?
		      PEER_TYPE_PEER:PEER_TYPE_CUSTOMER) == -1) {
	LOG_WARNING("warning: could not add peer AS%u to AS%u\n", uAS2, uAS1);
	continue;
      }
      if (as_add_peer(pAS2, uAS1, tAddr1,
		      (uRelation == REXFORD_REL_PEER_PEER)?
		      PEER_TYPE_PEER:PEER_TYPE_PROVIDER) == -1) {
	LOG_WARNING("warning: could not add peer AS%u to AS%u\n", uAS1, uAS2);
	continue;
      }
      
    }
    fclose(pFile);
  } else {
    LOG_SEVERE("Error: could not open topology file \"%s\"\n", pcFileName);
    iError= REXFORD_ERROR_OPEN;
  }
  tokenizer_destroy(&pTokenizer);
  return iError;
}

// ----- rexford_setup_policies -------------------------------------
/**
 *
 */
void rexford_setup_policies()
{
  uint16_t uAS1;
  SPeer * pPeer;
  uint8_t uPeerType;
  int iIndex;
  SFilter * pFilter;
  uint16_t uNumProviders;

  uAS1= 0;
  while (1) {
    if (AS[uAS1] != NULL) {
      uNumProviders= as_num_providers(AS[uAS1]);
      for (iIndex= 0; iIndex < ptr_array_length(AS[uAS1]->pPeers);
	   iIndex++) {
	pPeer= (SPeer *) AS[uAS1]->pPeers->data[iIndex];
	uPeerType= pPeer->uPeerType;

	/* Setup business-policies:
	 * ->customers: community remove COMM_PROV
	 *              community remove COMM_PEER
	 * <-customers: local-pref 100
	 * ->peers: deny if (community COMM_PROV) || (community COMM_PEER),
	 *          community remove COMM_PROV,
	 *          community remove COMM_PEER
	 * <-peers: community append COMM_PEER,
	 *          local-pref 80
	 * ->providers: deny if (community COMM_PROV) || (community COMM_PEER),
	 *              community remove COMM_PROV,
	 *              community remove COMM_PEER
	 * <-providers: community append COMM_PROV,
	 *              local-pref 60
	 */
	switch (uPeerType) {
	case PEER_TYPE_CUSTOMER:
	  // Provider input filter
	  pFilter= filter_create();
	  filter_add_rule(pFilter, NULL, filter_action_pref_set(PREF_CUST));
	  as_peer_set_in_filter(AS[uAS1], pPeer->tAddr, pFilter);
	  // Provider output filter
	  pFilter= filter_create();
	  filter_add_rule(pFilter, NULL, filter_action_comm_remove(COMM_PROV));
	  filter_add_rule(pFilter, NULL, filter_action_comm_remove(COMM_PEER));
	  as_peer_set_out_filter(AS[uAS1], pPeer->tAddr, pFilter);
	  break;
	case PEER_TYPE_PROVIDER:
	  // Customer input filter
	  pFilter= filter_create();
	  filter_add_rule(pFilter, NULL, filter_action_comm_append(COMM_PROV));
	  filter_add_rule(pFilter, NULL, filter_action_pref_set(PREF_PROV));
	  as_peer_set_in_filter(AS[uAS1], pPeer->tAddr, pFilter);
	  // Customer output filter
	  pFilter= filter_create();
	  filter_add_rule(pFilter, FTM_OR(filter_match_comm_contains(COMM_PROV),
					  filter_match_comm_contains(COMM_PEER)),
			  FTA_DENY);
	  filter_add_rule(pFilter, NULL, filter_action_comm_remove(COMM_PROV));
	  filter_add_rule(pFilter, NULL, filter_action_comm_remove(COMM_PEER));
	  as_peer_set_out_filter(AS[uAS1], pPeer->tAddr, pFilter);
	  break;
	case PEER_TYPE_PEER:
	  // Peer input filter
	  pFilter= filter_create();
	  filter_add_rule(pFilter, NULL, filter_action_comm_append(COMM_PEER));
	  filter_add_rule(pFilter, NULL, filter_action_pref_set(PREF_PEER));
	  as_peer_set_in_filter(AS[uAS1], pPeer->tAddr, pFilter);
	  // Peer output filter
	  pFilter= filter_create();
	  
	  if (uNumProviders > 0)
	    filter_add_rule(pFilter,
			    FTM_OR(filter_match_comm_contains(COMM_PROV),
				   filter_match_comm_contains(COMM_PEER)),
			    FTA_DENY);
	  filter_add_rule(pFilter, NULL, filter_action_comm_remove(COMM_PROV));
	  filter_add_rule(pFilter, NULL, filter_action_comm_remove(COMM_PEER));
	  as_peer_set_out_filter(AS[uAS1], pPeer->tAddr, pFilter);
	  break;
	default:
	  abort();
	}

      }
    }
    if (uAS1 < MAX_AS-1)
      uAS1++;
    else
      break;
  }
}

// ----- rexford_run ------------------------------------------------
/**
 *
 */
int rexford_run()
{
  int iIndex;

  for (iIndex= 0; iIndex < MAX_AS; iIndex++)
    if (AS[iIndex] != NULL)
      if (as_run(AS[iIndex]) != 0)
	return -1;
  return 0;
}

// ----- rexford_record_route ---------------------------------------
/**
 *
 */
int rexford_record_route(FILE * pStream, char * pcFileName, SPrefix sPrefix)
{
  FILE * pFileInput;
  char acFileLine[80];
  int iError= 0;
  uint16_t uAS;
  int iResult;
  SPath * pPath= NULL;

  if ((pFileInput= fopen(pcFileName, "r")) != NULL) {
    while ((!feof(pFileInput)) && (!iError)) {
      if (fgets(acFileLine, sizeof(acFileLine), pFileInput) == NULL)
	break;
      
      if ((strlen(acFileLine) > 0) && (acFileLine[0] == '*')) {
	
	if (sscanf(acFileLine, "*") != 0) {
	  iError= 1;
	  break;
	}
	
	uAS= 0;
	while (1) {
	  if (AS[uAS] != NULL) {
	    iResult= bgp_router_record_route(AS[uAS], sPrefix, &pPath, 0);
	    bgp_router_dump_recorded_route(pStream, AS[uAS], sPrefix,
					   pPath, iResult);
	    path_destroy(&pPath);
	  }
	  if (uAS == MAX_AS-1)
	    break;
	  else
	    uAS++;
	}
	
      } else {
	if (sscanf(acFileLine, "%hu", &uAS) != 1) {
	  iError= 1;
	  break;
	}

	// Check that source AS exists and that the mask length is valid
	if (AS[uAS] == NULL) {
	  iError= 1;
	  break;
	}
	
	// Record route
	iResult= bgp_router_record_route(AS[uAS], sPrefix, &pPath, 0);
	bgp_router_dump_recorded_route(pStream, AS[uAS], sPrefix,
				       pPath, iResult);
	path_destroy(&pPath);
	
      }
      
    }
    fclose(pFileInput);
  } else
    iError= 1;
  if (iError)
    return -1;
  return 0;
}

// ----- rexford_record_route_bm ------------------------------------
/**
 *
 */
int rexford_record_route_bm(FILE * pStream, char * pcFileName,
			    SPrefix sPrefix, uint8_t uBound)
{
  FILE * pFileInput;
  char acFileLine[80];
  int iError= 0;
  uint16_t uAS;
  int iResult;
  SPath * pPath= NULL;

  if ((pFileInput= fopen(pcFileName, "r")) != NULL) {
    while ((!feof(pFileInput)) && (!iError)) {
      if (fgets(acFileLine, sizeof(acFileLine), pFileInput) == NULL)
	break;
      
      if ((strlen(acFileLine) > 0) && (acFileLine[0] == '*')) {
	
	if (sscanf(acFileLine, "*") != 0) {
	  iError= 1;
	  break;
	}
	
	uAS= 0;
	while (1) {
	  if (AS[uAS] != NULL) {
	    iResult= bgp_router_record_route_bounded_match(AS[uAS], sPrefix,
							   uBound,
							   &pPath, 0);
	    bgp_router_dump_recorded_route(pStream, AS[uAS], sPrefix,
					   pPath, iResult);
	    path_destroy(&pPath);
	  }
	  if (uAS == MAX_AS-1)
	    break;
	  else
	    uAS++;
	}
	
      } else {
	if (sscanf(acFileLine, "%hu", &uAS) != 1) {
	  iError= 1;
	  break;
	}

	// Check that source AS exists and that the mask length is valid
	if (AS[uAS] == NULL) {
	  iError= 1;
	  break;
	}
	
	// Record route
	iResult= bgp_router_record_route_bounded_match(AS[uAS], sPrefix,
						       uBound,
						       &pPath, 0);
	bgp_router_dump_recorded_route(pStream, AS[uAS], sPrefix,
				       pPath, iResult);
	path_destroy(&pPath);
	
      }
      
    }
    fclose(pFileInput);
  } else
    iError= 1;
  if (iError)
    return -1;
  return 0;
}

// ----- rexford_route_dp_rule --------------------------------------
/**
 *
 */
int rexford_route_dp_rule(FILE * pStream, SPrefix sPrefix)
{
  int iIndex;

  for (iIndex= 0; iIndex < MAX_AS; iIndex++) {
    if (AS[iIndex] != NULL) {
      as_dump_rt_dp_rule(pStream, AS[iIndex], sPrefix);
    }
  }
  return 0;
}

/////////////////////////////////////////////////////////////////////
// INITIALIZATION AND FINALIZATION SECTION
/////////////////////////////////////////////////////////////////////

void _rexford_init() __attribute__((constructor));
void _rexford_destroy() __attribute__((destructor));

// ----- _rexford_init ----------------------------------------------
/**
 *
 */
void _rexford_init()
{
  int iIndex;

  for (iIndex= 0; iIndex < MAX_AS; iIndex++) {
    AS[iIndex]= NULL;
  }
  pASEdges= NULL;
}

// ----- _rexford_destroy -------------------------------------------
/**
 *
 */
void _rexford_destroy()
{
  ptr_array_destroy(&pASEdges);
}
