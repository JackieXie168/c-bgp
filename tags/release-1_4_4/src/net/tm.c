// ==================================================================
// @(#)tm.c
//
// Traffic matrix management.
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 23/01/2007
// @lastdate 20/11/2007
// ==================================================================
// TODO:
//   - what action must be taken in case of incomplete record-route
//     (UNREACH, ...)
//   - inbound interface (in-if) is not yet supported (field is
//     ignored)
// ==================================================================

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>

#include <libgds/str_util.h>
#include <libgds/tokenizer.h>
#include <net/network.h>
#include <net/record-route.h>
#include <net/tm.h>
#include <net/util.h>

// -----[ net_tm_perror ]--------------------------------------------
/**
 *
 */
void net_tm_perror(SLogStream * pStream, int iError)
{
  char * pcError= net_tm_strerror(iError);
  if (pcError != NULL)
    log_printf(pStream, pcError);
  else
    log_printf(pStream, "unknown error (%d)", iError);
}

// -----[ net_tm_strerror ]--------------------------------------------
/**
 *
 */
char * net_tm_strerror(int iError)
{
  switch (iError) {
  case NET_TM_SUCCESS:
    return "success";
  case NET_TM_ERROR_UNEXPECTED:
    return "unexpected error";
  case NET_TM_ERROR_OPEN:
    return "failed to open file";
  case NET_TM_ERROR_NUM_PARAMS:
    return "not enough fields";
  case NET_TM_ERROR_INVALID_SRC:
    return "invalid source";
  case NET_TM_ERROR_UNKNOWN_SRC:
    return "source not found";
  case NET_TM_ERROR_INVALID_DST:
    return "invalid destination";
  case NET_TM_ERROR_INVALID_LOAD:
    return "invalid load";
  }
  return NULL;
}

// -----[ net_tm_parser ]--------------------------------------------
/**
 * Parse a traffic matrix.
 *
 * Format:
 *   <in-rt> <in-if> <dst-pfx> <V> [<tos>]
 *
 * where <in-rt>   is the source router
 *       <in-if>   is it's source interface
 *       <dst-pfx> is the destination
 *       <load>    is the volume of traffic
 *       <tos>     is the TOS (optional)
 */
int net_tm_parser(FILE * pStream)
{
  char acFileLine[80];
  STokenizer * pTokenizer;
  STokens * pTokens;
  int iError= NET_TM_SUCCESS;
  char * pcValue;
  unsigned int uValue;
  uint32_t uLineNumber= 0;
  net_addr_t tSrcAddr;
  SNetNode * pSrcNode;
  //SPrefix sSrcIface;
  //SNetLink * pSrcLink;
  SNetDest sDst;
  net_link_load_t tLoad;
  net_tos_t tTOS= 0;
  SNetRecordRouteInfo * pRRInfo;

  pTokenizer= tokenizer_create(" \t", 0, NULL, NULL);
  
  while ((!feof(pStream)) && (iError == NET_TM_SUCCESS)) {
    if (fgets(acFileLine, sizeof(acFileLine), pStream) == NULL)
      break;

    uLineNumber++;

    // Split the line in tokens
    if (tokenizer_run(pTokenizer, acFileLine)) {
      iError= NET_TM_ERROR_UNEXPECTED;
      break;
    }

    pTokens= tokenizer_get_tokens(pTokenizer);

    // We should have at least 4 tokens
    if (tokens_get_num(pTokens) < 4) {
      iError= NET_TM_ERROR_NUM_PARAMS;
      break;
    }

    // Source node
    pcValue= tokens_get_string_at(pTokens, 0);
    if (str2address(pcValue, &tSrcAddr) < 0) {
      iError= NET_TM_ERROR_INVALID_SRC;
      break;
    }
    pSrcNode= network_find_node(tSrcAddr);
    if (pSrcNode == NULL) {
      iError= NET_TM_ERROR_UNKNOWN_SRC;
      break;
    }
    
    // Source interface
    pcValue= tokens_get_string_at(pTokens, 1);

    // Destination prefix
    pcValue= tokens_get_string_at(pTokens, 2);
    if (ip_string_to_dest(pcValue, &sDst) < 0) {
      iError= NET_TM_ERROR_INVALID_DST;
      break;
    }

    // Load (volume of traffic)
    pcValue= tokens_get_string_at(pTokens, 3);
    if (str_as_uint(pcValue, &uValue) < 0) {
      iError= NET_TM_ERROR_INVALID_LOAD;
      break;
    }
    tLoad= uValue;

    // Optional TOS ?
    
    // Load traffic
    pRRInfo= node_record_route(pSrcNode, sDst, tTOS,
			       NET_RECORD_ROUTE_OPTION_LOAD, tLoad);
    if (pRRInfo == NULL) {
      iError= NET_TM_ERROR_UNEXPECTED;
      break;
    }
    /** IN THE FUTURE, WE SHOULD CHECK TRAFFIC MATRIX ERRORS
    if (pRRInfo->iResult == NET_RECORD_ROUTE_TO_HOST) {
      iError= NET_TM_ERROR_ROUTE;
      break;
    }
    */
    net_record_route_info_destroy(&pRRInfo);

  }
  
  tokenizer_destroy(&pTokenizer);
  return iError;
}

// -----[ net_tm_load ]----------------------------------------------
/**
 * Load a traffic matrix.
 */
int net_tm_load(const char * pcFileName)
{
  FILE * pFile;
  int iResult;

  pFile= fopen(pcFileName, "r");
  if (pFile == NULL)
    return NET_TM_ERROR_OPEN;

  iResult= net_tm_parser(pFile);

  fclose(pFile);
  return iResult;
}
