// ==================================================================
// @(#)ntf.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 24/03/2005
// @lastdate 03/03/2006
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>

#include <libgds/log.h>
#include <libgds/tokenizer.h>

#include <net/network.h>
#include <net/node.h>
#include <net/ntf.h>

// -----[ ntf_load ]-------------------------------------------------
/**
 * Load the given NTF file into the the topology.
 */
int ntf_load(char * pcFileName)
{
  FILE * pFile;
  char acFileLine[80];
  int iError= NTF_SUCCESS;
  STokenizer * pTokenizer;
  STokens * pTokens;
  uint32_t uLineNumber= 0;
  char * pcNode1, * pcNode2;
  char * pcEndPtr;
  net_addr_t tNode1, tNode2;
  uint32_t uWeight;
  net_link_delay_t tDelay;
  SNetNode * pNode1, * pNode2;
  SNetLink * pLink;

  pTokenizer= tokenizer_create(" \t", 0, NULL, NULL);

  if ((pFile= fopen(pcFileName, "r")) != NULL) {
    
    while ((!feof(pFile)) && (iError == NTF_SUCCESS)) {
      if (fgets(acFileLine, sizeof(acFileLine), pFile) == NULL)
	break;

      uLineNumber++;
      
      if (tokenizer_run(pTokenizer, acFileLine)) {
	iError= NTF_ERROR_UNEXPECTED;
	LOG_ERR(LOG_LEVEL_SEVERE, "Error: unexpected parse error in NTF file, line %u\n",
		   uLineNumber);
	break;
      }

      pTokens= tokenizer_get_tokens(pTokenizer);

      if (tokens_get_num(pTokens) < 3) {
	LOG_ERR(LOG_LEVEL_SEVERE, "Error: not enouh parameters in NTF file, line %u\n",
		   uLineNumber);
	iError= NTF_ERROR_NUM_PARAMS;
	break;	
      }

      // Check IP address of first node
      pcNode1= tokens_get_string_at(pTokens, 0);
      if ((ip_string_to_address(pcNode1, &pcEndPtr, &tNode1) != 0) ||
	  (*pcEndPtr != '\0')) {
	LOG_ERR(LOG_LEVEL_SEVERE, "Error: invalid IP address for first node (%s), line %u\n",
		   pcNode1, uLineNumber);
	iError= NTF_ERROR_INVALID_ADDRESS;
	break;
      }

      // Check IP address of second node
      pcNode2= tokens_get_string_at(pTokens, 1);
      if ((ip_string_to_address(pcNode2, &pcEndPtr, &tNode2) != 0) ||
	  (*pcEndPtr != '\0')) {
	LOG_ERR(LOG_LEVEL_SEVERE, "Error: invalid IP address for second node (%s), line %u\n",
		   pcNode1, uLineNumber);
	iError= NTF_ERROR_INVALID_ADDRESS;
	break;
      }

      // Check IGP metric
      if (tokens_get_uint_at(pTokens, 2, &uWeight)) {
	LOG_ERR(LOG_LEVEL_SEVERE, "Error: invalid IGP metric (%s), line %u\n",
		   tokens_get_string_at(pTokens, 2), uLineNumber);
	iError= NTF_ERROR_INVALID_WEIGHT;
	break;
      }
      
      // Check delay (if provided)
      tDelay= uWeight;
      if (tokens_get_num(pTokens) > 3) {
	if (tokens_get_uint_at(pTokens, 3, &tDelay)) {
	  LOG_ERR(LOG_LEVEL_SEVERE, "Error: invalid delay (%s), line %u\n",
		     tokens_get_string_at(pTokens, 3), uLineNumber);
	  iError= NTF_ERROR_INVALID_DELAY;
	  break;
	}
      }

      pNode1= network_find_node(tNode1);
      if (pNode1 == NULL) {
	pNode1= node_create(tNode1);
	network_add_node(pNode1);
      }

      pNode2= network_find_node(tNode2);
      if (pNode2 == NULL) {
	pNode2= node_create(tNode2);
	network_add_node(pNode2);
      }

      node_add_link_to_router(pNode1, pNode2, tDelay, 1);
      pLink= node_find_link_to_router(pNode1, tNode2);
      pLink->uIGPweight= uWeight;

    }

    fclose(pFile);

  } else {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: could not open NTF file \"%s\"\n", pcFileName);
    iError= NTF_ERROR_OPEN;    
  }

  tokenizer_destroy(&pTokenizer);

  return 0;
}

// -----[ ntf_save ]-------------------------------------------------
/**
 * Save the topology in an NTF file.
 */
int ntf_save(char * pcFileName)
{
  return -1;
}
