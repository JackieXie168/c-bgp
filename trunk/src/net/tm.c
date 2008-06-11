// ==================================================================
// @(#)tm.c
//
// Traffic matrix management.
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 23/01/2007
// $Id: tm.c,v 1.4 2008-06-11 15:13:45 bqu Exp $
// ==================================================================
// TODO:
//   - what action must be taken in case of incomplete record-route
//     (UNREACH, ...)
//   - inbound interface (in-if) is not yet supported (field is
//     ignored)
//   - TOS field is not yet supported (field is ignored)
//   - there is some redundancy with the NetFlow handling regarding
//     how record-route is configured and performed.
// ==================================================================

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>

#include <libgds/str_util.h>
#include <libgds/tokenizer.h>
#include <net/network.h>
#include <net/icmp.h>
#include <net/icmp_options.h>
#include <net/tm.h>
#include <net/util.h>

#define __VERBOSE

// -----[ net_tm_perror ]--------------------------------------------
/**
 *
 */
void net_tm_perror(SLogStream * stream, int error)
{
  char * error_str= net_tm_strerror(error);
  if (error_str != NULL)
    log_printf(stream, error_str);
  else
    log_printf(stream, "unknown error (%d)", error);
}

// -----[ net_tm_strerror ]--------------------------------------------
/**
 *
 */
char * net_tm_strerror(int error)
{
  switch (error) {
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
int net_tm_parser(FILE * stream)
{
  char acFileLine[80];
  STokenizer * pTokenizer;
  STokens * pTokens;
  int error= NET_TM_SUCCESS;
  char * pcValue;
  unsigned int uValue;
  uint32_t uLineNumber= 0;
  net_addr_t src_addr;
  net_node_t * node;
  //SPrefix sSrcIface;
  //net_iface_t * pSrcLink;
  SNetDest sDst;
  net_link_load_t load;
  //net_tos_t tos= 0;
  network_t * network= network_get_default();
  uint8_t options= ICMP_RR_OPTION_LOAD;

  pTokenizer= tokenizer_create(" \t", 0, NULL, NULL);
  
  while ((!feof(stream)) && (error == NET_TM_SUCCESS)) {
    if (fgets(acFileLine, sizeof(acFileLine), stream) == NULL)
      break;

    uLineNumber++;

    // Split the line in tokens
    if (tokenizer_run(pTokenizer, acFileLine)) {
      error= NET_TM_ERROR_UNEXPECTED;
      break;
    }

    pTokens= tokenizer_get_tokens(pTokenizer);

    // We should have at least 4 tokens
    if (tokens_get_num(pTokens) < 4) {
      error= NET_TM_ERROR_NUM_PARAMS;
      break;
    }

    // Source node
    pcValue= tokens_get_string_at(pTokens, 0);
    if (str2address(pcValue, &src_addr) < 0) {
      error= NET_TM_ERROR_INVALID_SRC;
      break;
    }
    node= network_find_node(network, src_addr);
    if (node == NULL) {
      error= NET_TM_ERROR_UNKNOWN_SRC;
      break;
    }
    
    // Source interface
    pcValue= tokens_get_string_at(pTokens, 1);
    // Not used for forwarding (yet)

    // Destination prefix
    pcValue= tokens_get_string_at(pTokens, 2);
    if (ip_string_to_dest(pcValue, &sDst) < 0) {
      error= NET_TM_ERROR_INVALID_DST;
      break;
    }

    // Load (volume of traffic)
    pcValue= tokens_get_string_at(pTokens, 3);
    if (str_as_uint(pcValue, &uValue) < 0) {
      error= NET_TM_ERROR_INVALID_LOAD;
      break;
    }
    load= uValue;

    // Optional TOS ?
    // --> Not supported (yet)
    
    if (sDst.tType == NET_DEST_PREFIX)
      options|= ICMP_RR_OPTION_ALT_DEST;

#ifdef __VERBOSE
    fprintf(stderr, "load traffic from ");
    ip_address_dump(pLogErr, node->addr);
    fprintf(stderr, " to ");
    ip_dest_dump(pLogErr, sDst);
    fprintf(stderr, ", volume=%d\n", load);
#endif /* __VERBOSE */

    // Trace route and load traffic
    ip_trace_t * trace;
    error= icmp_record_route(node, sDst.uDest.tAddr,
			     &sDst.uDest.sPrefix,
			     255, options, &trace, load);
    if (error != ESUCCESS) {
      error= NET_TM_ERROR_UNEXPECTED;
      break;
    }

#ifdef __VERBOSE
    fprintf(stderr, "--> ");
    ip_trace_dump(pLogErr, trace, IP_TRACE_DUMP_LENGTH|IP_TRACE_DUMP_SUBNETS);
    fprintf(stderr, "\n");
#endif /* __VERBOSE */

    ip_trace_destroy(&trace);
  }
  
  tokenizer_destroy(&pTokenizer);
  return error;
}

// -----[ net_tm_load ]----------------------------------------------
/**
 * Load a traffic matrix.
 */
int net_tm_load(const char * filename)
{
  FILE * pFile;
  int error;

  pFile= fopen(filename, "r");
  if (pFile == NULL)
    return NET_TM_ERROR_OPEN;

  error= net_tm_parser(pFile);

  fclose(pFile);
  return error;
}
