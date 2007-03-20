// ==================================================================
// @(#)ntf.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 24/03/2005
// @lastdate 23/01/2007
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <assert.h>
#include <stdio.h>

#include <libgds/log.h>
#include <libgds/tokenizer.h>

#include <net/network.h>
#include <net/node.h>
#include <net/ntf.h>
#include <net/util.h>

// -----[ ntf_load ]-------------------------------------------------
/**
 * Load the given NTF file into the topology.
 */
int ntf_load(char * pcFileName)
{
  FILE * pFile;
  char acFileLine[80];
  int iError= NTF_SUCCESS;
  STokenizer * pTokenizer;
  STokens * pTokens;
  uint32_t uLineNumber= 0;
  char * pcValue;
  net_addr_t tNode1, tNode2;
  unsigned int uWeight;
  unsigned int uDelay;
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
      pcValue= tokens_get_string_at(pTokens, 0);
      if (str2address(pcValue, &tNode1) < 0) {
	LOG_ERR(LOG_LEVEL_SEVERE, "Error: invalid IP address for first node (%s), line %u\n",
		   pcValue, uLineNumber);
	iError= NTF_ERROR_INVALID_ADDRESS;
	break;
      }

      // Check IP address of second node
      pcValue= tokens_get_string_at(pTokens, 1);
      if (str2address(pcValue, &tNode2) < 0) {
	LOG_ERR(LOG_LEVEL_SEVERE, "Error: invalid IP address for second node (%s), line %u\n",
		   pcValue, uLineNumber);
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
      tDelay= (net_link_delay_t) uWeight;
      if (tokens_get_num(pTokens) > 3) {
	if (tokens_get_uint_at(pTokens, 3, &uDelay)) {
	  LOG_ERR(LOG_LEVEL_SEVERE, "Error: invalid delay (%s), line %u\n",
		     tokens_get_string_at(pTokens, 3), uLineNumber);
	  iError= NTF_ERROR_INVALID_DELAY;
	  break;
	}
        tDelay= (net_link_delay_t) uDelay;
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

      node_add_link_ptp(pNode1, pNode2, tDelay, 0,
			NET_LINK_DEFAULT_DEPTH,
			NET_LINK_BIDIR);
      assert((pLink= node_find_link_ptp(pNode1, tNode2)) != NULL);
      net_link_set_weight(pLink, 0, uWeight);
      assert((pLink= node_find_link_ptp(pNode2, tNode1)) != NULL);
      net_link_set_weight(pLink, 0, uWeight);

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
