// ==================================================================
// @(#)import_meulle.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 15/10/2007
// @lastdate 16/10/2007
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif /* HAVE_CONFIG_H */

#include <assert.h>
#include <string.h>

#include <libgds/log.h>
#include <libgds/str_util.h>
#include <libgds/tokenizer.h>

#include <bgp/as.h>
#include <bgp/as_t.h>
#include <bgp/as-level.h>
#include <bgp/import_meulle.h>
#include <bgp/peer.h>
#include <bgp/rexford.h>
#include <net/network.h>
#include <net/node.h>
#include <net/protocol.h>

// -----[ meulle_parser ]-------------------------------------------
/**
 * Load an AS-level topology in the "Meulle" format. The
 * file format is as follows. Each line describes a directed
 * relationship between a two ASes:
 *   AS<ASi-number> \t AS<ASj-number> \t <peering-type>
 * where peering-type can be
 *   PEER for a peer-to-peer relationship
 *   P2C  for a provider (ASi) to customer (ASj) relationship
 *   C2P  for a customer (ASi) to provider (ASj) relationship
 * A relationship among two ASes is described in one direction only.
 */
int meulle_parser(FILE * pStream, SASLevelTopo * pTopo,
		  unsigned int * puLineNumber)
{
  char acFileLine[80];
  char * pcAS1, * pcAS2, * pcRelation;
  unsigned int uAS1, uAS2;
  int iError= 0;
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
    
    // Get and check mandatory parameters
    if (tokens_get_num(pTokens) != 3) {
      iError= ASLEVEL_ERROR_NUM_PARAMS;
      break;
    }

    // Get and check ASNs
    pcAS1= tokens_get_string_at(pTokens, 0);
    if (strncmp(pcAS1, "AS", 2) ||
	(str_as_uint(pcAS1+2, &uAS1) != 0) ||
	(uAS1 >= MAX_AS)) {
      iError= ASLEVEL_ERROR_INVALID_ASNUM;
      break;
    }
    pcAS2= tokens_get_string_at(pTokens, 1);
    if (strncmp(pcAS2, "AS", 2) ||
	(str_as_uint(pcAS2+2, &uAS2) != 0) ||
	(uAS2 >= MAX_AS)) {
      iError= ASLEVEL_ERROR_INVALID_ASNUM;
      break;
    }

    // Get and check business relationship
    pcRelation= tokens_get_string_at(pTokens, 2);
    if (!strcmp(pcRelation, "PEER")) {
      tPeerType= ASLEVEL_PEER_TYPE_PEER;
    } else if (!strcmp(pcRelation, "P2C")) {
      tPeerType= ASLEVEL_PEER_TYPE_CUSTOMER;
    } else if (!strcmp(pcRelation, "C2P")) {
      tPeerType= ASLEVEL_PEER_TYPE_PROVIDER;
    } else {
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
    
    // Add link
    iError= aslevel_as_add_link(pDomain1, pDomain2, tPeerType, NULL);
    if (iError	!= ASLEVEL_SUCCESS)
      break;
    
  }

  tokenizer_destroy(&pTokenizer);
  return iError;
}
