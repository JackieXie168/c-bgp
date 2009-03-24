// ==================================================================
// @(#)caida.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 30/04/2007
// $Id: caida.c,v 1.1 2009-03-24 13:39:08 bqu Exp $
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <assert.h>
#include <string.h>

#include <libgds/stream.h>
#include <libgds/tokenizer.h>

#include <net/network.h>
#include <net/node.h>
#include <net/protocol.h>
#include <net/util.h>

#include <bgp/as.h>
#include <bgp/aslevel/as-level.h>
#include <bgp/aslevel/caida.h>
#include <bgp/filter/filter.h>
#include <bgp/peer.h>

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
int caida_parser(FILE * file, as_level_topo_t * topo,
		 unsigned int * line_number)
{
  char line[80];
  asn_t asn1, asn2;
  int relation;
  int error= 0;
  gds_tokenizer_t * tokenizer;
  const gds_tokens_t * tokens;
  as_level_domain_t * domain1, * domain2;
  peer_type_t peer_type;

  // Parse input file
  tokenizer= tokenizer_create(" \t", NULL, NULL);
  *line_number= 0;

  while ((!feof(file)) && (!error)) {
    if (fgets(line, sizeof(line), file) == NULL)
      break;
    (*line_number)++;

    // Skip comments starting with '#'
    if (line[0] == '#')
      continue;
    
    // Parse a single line
    if (tokenizer_run(tokenizer, line) != TOKENIZER_SUCCESS) {
      error= ASLEVEL_ERROR_UNEXPECTED;
      break;
    }
    
    tokens= tokenizer_get_tokens(tokenizer);
    
    // Get and check mandatory parameters
    if (tokens_get_num(tokens) != 3) {
      error= ASLEVEL_ERROR_NUM_PARAMS;
      break;
    }
    
    // Get AS1 and AS2
    if (str2asn(tokens_get_string_at(tokens, 0), &asn1) ||
	str2asn(tokens_get_string_at(tokens, 1), &asn2)) {
      error= ASLEVEL_ERROR_INVALID_ASNUM;
      break;
    }
    
    // Get relationship
    if ((tokens_get_int_at(tokens, 2, &relation) != 0) ||
	(_caida_relation_to_peer_type(relation, &peer_type) != 0)) {
      error= ASLEVEL_ERROR_INVALID_RELATION;
      break;
    }
    
    // Add/get domain 1
    if (!(domain1= aslevel_topo_get_as(topo, asn1)) &&
	!(domain1= aslevel_topo_add_as(topo, asn1))) {
      error= ASLEVEL_ERROR_UNEXPECTED;
      break;
    }
    
    // Add/get domain 2
    if (!(domain2= aslevel_topo_get_as(topo, asn2)) &&
	!(domain2= aslevel_topo_add_as(topo, asn2))) {
      error= ASLEVEL_ERROR_UNEXPECTED;
      break;
    }
    
    // Add link in one direction
    error= aslevel_as_add_link(domain1, domain2, peer_type, NULL);
    if (error != ASLEVEL_SUCCESS)
      break;
    
  }
  tokenizer_destroy(&tokenizer);

  return error;
}
