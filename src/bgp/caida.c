// ==================================================================
// @(#)caida.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 30/04/2007
// @lastdate 16/10/2007
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
#include <bgp/filter.h>
#include <bgp/peer.h>
#include <bgp/caida.h>
#include <net/network.h>
#include <net/node.h>
#include <net/protocol.h>

// -----[ _caida_relation_to_peer_type ]-----------------------------
static inline int _caida_relation_to_peer_type(int iRelation,
					       peer_type_t * ptPeerType)
{
  switch (iRelation) {
  case CAIDA_REL_CUST_PROV:
    *ptPeerType= ASLEVEL_PEER_TYPE_PROVIDER;
    return 0;
  case CAIDA_REL_PROV_CUST:
    *ptPeerType= ASLEVEL_PEER_TYPE_CUSTOMER;
    return 0;
  case CAIDA_REL_PEER_PEER:
    *ptPeerType= ASLEVEL_PEER_TYPE_PEER;
    return 0;
  case CAIDA_REL_SIBLINGS:
    *ptPeerType= ASLEVEL_PEER_TYPE_SIBLING;
    return 0;
  }
  return -1;
}

// -----[ caida_parser ]---------------------------------------------
/**
 * Load an AS-level topology in the CAIDA format. The file format is
 * as follows. Each line describes a directed relationship between a
 * two ASes:
 *   <AS1-number> <AS2-number> <peering-type>
 * where peering type can be
 *  -1 for a customer (AS1) to provider (AS2) relationship
 *   0 for a peer-to-peer relationship
 *   1 for a provider (AS1) to customer (AS2) relationship
 *   2 for a sibling relationship
 * A relationship among two ASes is described in both directions.
 */
int caida_parser(FILE * pStream, SASLevelTopo * pTopo,
		 unsigned int * puLineNumber)
{
  char acFileLine[80];
  unsigned int uAS1, uAS2;
  int iRelation;
  int iError= 0;
  STokenizer * pTokenizer;
  STokens * pTokens;
  SASLevelDomain * pDomain1, * pDomain2;
  peer_type_t tPeerType;

  // Parse input file
  pTokenizer= tokenizer_create(" \t", 0, NULL, NULL);
  *puLineNumber= 0;

  while ((!feof(pStream)) && (!iError)) {
    if (fgets(acFileLine, sizeof(acFileLine), pStream) == NULL)
      break;
    (*puLineNumber)++;

    // Skip comments starting with '#'
    if (acFileLine[0] == '#')
      continue;
    
    // Parse a single line
    if (tokenizer_run(pTokenizer, acFileLine) != TOKENIZER_SUCCESS) {
      iError= ASLEVEL_ERROR_UNEXPECTED;
      break;
    }
    
    pTokens= tokenizer_get_tokens(pTokenizer);
    
    // Get and check mandatory parameters
    if (tokens_get_num(pTokens) != 3) {
      iError= ASLEVEL_ERROR_NUM_PARAMS;
      break;
    }
    
    // Get AS1 and AS2
    if ((tokens_get_uint_at(pTokens, 0, &uAS1) != 0) ||
	(tokens_get_uint_at(pTokens, 1, &uAS2) != 0) ||
	(uAS1 >= MAX_AS) || (uAS2 >= MAX_AS)) {
      iError= ASLEVEL_ERROR_INVALID_ASNUM;
      break;
    }
    
    // Get relationship
    if ((tokens_get_int_at(pTokens, 2, &iRelation) != 0) ||
	(_caida_relation_to_peer_type(iRelation, &tPeerType) != 0)) {
      iError= ASLEVEL_ERROR_INVALID_RELATION;
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
    
    // Add link in one direction
    iError= aslevel_as_add_link(pDomain1, pDomain2, tPeerType, NULL);
    if (iError != ASLEVEL_SUCCESS)
      break;
    
  }
  tokenizer_destroy(&pTokenizer);

  return iError;
}
