// ==================================================================
// @(#)rexford.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 28/07/2003
// @lastdate 14/05/2007
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <assert.h>
#include <string.h>

#include <libgds/log.h>
#include <libgds/tokenizer.h>

#include <bgp/as.h>
#include <bgp/as_t.h>
#include <bgp/as-level.h>
#include <bgp/peer.h>
#include <bgp/rexford.h>
#include <net/network.h>
#include <net/node.h>
#include <net/protocol.h>

// -----[ _rexford_relation_to_peer_type ]-----------------------------
static inline int _rexford_relation_to_peer_type(int iRelation,
						 peer_type_t * ptPeerType)
{
  switch (iRelation) {
  case REXFORD_REL_PROV_CUST:
    *ptPeerType= ASLEVEL_PEER_TYPE_CUSTOMER;
    return 0;
  case REXFORD_REL_PEER_PEER:
    *ptPeerType= ASLEVEL_PEER_TYPE_PEER;
    return 0;
  }
  return -1;
}

// -----[ rexford_parser ]-------------------------------------------
/**
 * Load an AS-level topology in the Subramanian et al format. The
 * file format is as follows. Each line describes a directed
 * relationship between a two ASes:
 *   <AS1-number> <AS2-number> <peering-type>
 * where peering type can be
 *   0 for a peer-to-peer relationship
 *   1 for a provider (AS1) to customer (AS2) relationship
 * A relationship among two ASes is described in one direction only.
 */
int rexford_parser(FILE * pStream, SASLevelTopo * pTopo,
		   unsigned int * puLineNumber)
{
  char acFileLine[80];
  unsigned int uAS1, uAS2, uRelation;
  int iError= 0;
  unsigned int uDelay;
  net_link_delay_t tDelay;
  STokenizer * pTokenizer;
  STokens * pTokens;
  SASLevelDomain * pDomain1, * pDomain2;
  peer_type_t tPeerType;

  *puLineNumber= 0;

  // Parse input file
  pTokenizer= tokenizer_create(" \t", 0, NULL, NULL);
  
  while ((!feof(pStream)) && (!iError)) {
    if (fgets(acFileLine, sizeof(acFileLine), pStream) == NULL)
      break;
    (*puLineNumber)++;
    
    // Skip comments starting with '#'
    if (acFileLine[0] == '#')
      continue;
    
    if (tokenizer_run(pTokenizer, acFileLine) != TOKENIZER_SUCCESS) {
      iError= ASLEVEL_ERROR_UNEXPECTED;
      break;
    }
    
    pTokens= tokenizer_get_tokens(pTokenizer);
    
    // Set default value for optional parameters
    tDelay= 0;
    
    // Get and check mandatory parameters
    if (tokens_get_num(pTokens) < 3) {
      iError= ASLEVEL_ERROR_NUM_PARAMS;
      break;
    }

    // Get and check ASNs
    if ((tokens_get_uint_at(pTokens, 0, &uAS1) != 0) ||
	(tokens_get_uint_at(pTokens, 1, &uAS2) != 0) ||
	(uAS1 >= MAX_AS) || (uAS2 >= MAX_AS)) {
      iError= ASLEVEL_ERROR_INVALID_ASNUM;
      break;
    }
    
    // Get and check business relationship
    if ((tokens_get_uint_at(pTokens, 2, &uRelation) != 0) ||
	(_rexford_relation_to_peer_type(uRelation, &tPeerType) != 0)) {
      iError= ASLEVEL_ERROR_INVALID_RELATION;
      break;
    }
    
    // Get optional parameters
    if (tokens_get_num(pTokens) > 3) {
      if (tokens_get_uint_at(pTokens, 3, &uDelay) != 0) {
	iError= ASLEVEL_ERROR_INVALID_DELAY;
	break;
      }
      tDelay= (net_link_delay_t) uDelay;
    }
    
    // Limit number of parameters
    if (tokens_get_num(pTokens) > 4) {
      LOG_ERR(LOG_LEVEL_SEVERE,
	      "Error: too many arguments in topology, line %u\n",
	      *puLineNumber);
      iError= ASLEVEL_ERROR_NUM_PARAMS;
      break;
    }
    
    // Add/get domain 1
    if (!(pDomain1= aslevel_topo_get_as(pTopo, uAS1)) &&
	!(pDomain1= aslevel_topo_add_as(pTopo, uAS1))) {
      iError= ASLEVEL_ERROR_UNEXPECTED;
      break;
    }
    
    // Add/get domain 2
    if (!(pDomain2= aslevel_topo_get_as(pTopo, uAS2)) &&
	!(pDomain2= aslevel_topo_add_as(pTopo, uAS2))) {
      iError= ASLEVEL_ERROR_UNEXPECTED;
      break;
    }
    
    // Add link in both directions
    if ((aslevel_as_add_link(pDomain1, pDomain2, tPeerType) == NULL) ||
	(aslevel_as_add_link(pDomain2, pDomain1,
			     aslevel_reverse_relation(tPeerType)) == NULL)) {
      iError= ASLEVEL_ERROR_DUPLICATE_LINK;
      break;
    }
    
  }

  tokenizer_destroy(&pTokenizer);
  return iError;
}
