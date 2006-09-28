// ==================================================================
// @(#)mrtd.c
//
// Interface with MRT data. The importation through the libbgpdump
// library was inspired from Dan Ardelean's example code.
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @author Dan Ardelean (dan@ripe.net, dardelea@cs.purdue.edu)
// @author Sebastien Tandel (standel@info.ucl.ac.be)
// 
// @date 20/02/2004
// @lastdate 28/09/2006
// ==================================================================
// Future changes:
// - move attribute parsers in corresponding sections
// - move neighbor auto-configuration outside mrtd.c

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <libgds/tokenizer.h>
#include <libgds/tokens.h>

#include <bgp/as_t.h>
#include <bgp/as.h>
#include <bgp/auto-config.h>
#include <bgp/filter.h>
#include <bgp/peer.h>
#include <bgp/comm.h>
#include <bgp/mrtd.h>
#include <bgp/path.h>
#include <bgp/path_segment.h>
#include <bgp/route.h>
#include <bgp/routes_list.h>
#include <net/network.h>
#include <net/node.h>
#include <net/prefix.h>
#include <net/protocol.h>

#ifdef HAVE_BGPDUMP
# include <sys/types.h>
# include <sys/socket.h>
# include <arpa/inet.h>
# include <external/bgpdump_lib.h>
# include <external/bgpdump_formats.h>
# include <netinet/in.h>
#endif

static STokenizer * pLineTokenizer= NULL;
static STokenizer * pCommTokenizer= NULL;
static STokenizer * pPathTokenizer= NULL;
static STokenizer * pSegmentTokenizer= NULL;

// ----- mrtd_create_path_segment -----------------------------------
SPathSegment * mrtd_create_path_segment(char * pcPathSegment)
{
  int iIndex;
  STokens * pTokens;
  SPathSegment * pSegment;
  unsigned long ulASNum;

  if (pSegmentTokenizer == NULL)
    pSegmentTokenizer= tokenizer_create(" ", 0, NULL, NULL);

  if (tokenizer_run(pSegmentTokenizer, pcPathSegment) != TOKENIZER_SUCCESS) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: parse error in 'mrtd_create_path_segment'\n");
    return NULL;
  }

  pTokens= tokenizer_get_tokens(pSegmentTokenizer);
  pSegment= path_segment_create(AS_PATH_SEGMENT_SET, 0);
  for (iIndex= tokens_get_num(pTokens); iIndex > 0; iIndex--) {
    if (tokens_get_ulong_at(pTokens, iIndex-1, &ulASNum) || (ulASNum > 65535)) {
      LOG_ERR(LOG_LEVEL_SEVERE, "Error: invalid AS-Num \"%s\"\n",
		 tokens_get_string_at(pTokens, iIndex-1));
      path_segment_destroy(&pSegment);
      break;
    }
    path_segment_add(&pSegment, ulASNum);
  }
  return pSegment;
}

// ----- mrtd_create_path -------------------------------------------
SBGPPath * mrtd_create_path(char * pcPath)
{
  int iIndex;
  STokens * pPathTokens;
  SBGPPath * pPath= NULL;
  char * pcSegment;
  SPathSegment * pSegment;
  unsigned long ulASNum;

  if (pPathTokenizer == NULL)
    pPathTokenizer= tokenizer_create(" ", 0, "[", "]");

  if (tokenizer_run(pPathTokenizer, pcPath) != TOKENIZER_SUCCESS)
    return NULL;

  pPathTokens= tokenizer_get_tokens(pPathTokenizer);

  pPath= path_create();
  for (iIndex= tokens_get_num(pPathTokens); iIndex > 0; iIndex--) {
    pcSegment= tokens_get_string_at(pPathTokens, iIndex-1);
    if (!tokens_get_ulong_at(pPathTokens, iIndex-1, &ulASNum)) {
      if (ulASNum > 65535) {
	LOG_ERR(LOG_LEVEL_SEVERE, "Error: not a valid AS-Num \"%s\"\n",
		   pcSegment);
	path_destroy(&pPath);
	break;
      }
      path_append(&pPath, ulASNum);
    } else {
      pSegment= mrtd_create_path_segment(pcSegment);
      if (pSegment == NULL) {
	LOG_ERR(LOG_LEVEL_SEVERE,
		"Error: not a valid path segment \"%s\"\n",
		pcSegment);
	path_destroy(&pPath);
	break;
      }
      path_add_segment(pPath, pSegment);
    }
  }

  return pPath;
}

// ----- mrtd_create_communities ------------------------------------
SCommunities * mrtd_create_communities(char * pcCommunities)
{
  int iIndex;
  SCommunities * pComm= NULL;
  STokens * pTokens;
  comm_t uComm;

  if (pCommTokenizer == NULL)
    pCommTokenizer= tokenizer_create(" ", 0, NULL, NULL);

  if (tokenizer_run(pCommTokenizer, pcCommunities) != TOKENIZER_SUCCESS)
    return NULL;

  pTokens= tokenizer_get_tokens(pCommTokenizer);

  pComm= comm_create();
  for (iIndex= 0; iIndex < tokens_get_num(pTokens); iIndex++) {
    if (comm_from_string(tokens_get_string_at(pTokens, iIndex), &uComm)) {
      LOG_ERR(LOG_LEVEL_SEVERE, "Error: not a valid community \"%s\"\n",
		 tokens_get_string_at(pTokens, iIndex));
      comm_destroy(&pComm);
      return NULL;
    }
    comm_add(&pComm, uComm);
  }
  
  return pComm;
}

// ----- mrtd_get_origin --------------------------------------------
/**
 * This function converts the origin field of an MRT record to the
 * route origin. If the route origin is unknown, -1 is returned.
 */
bgp_origin_t mrtd_get_origin(char * pcField)
{
  if (!strcmp(pcField, "IGP")) {
    return ROUTE_ORIGIN_IGP;
  } else if (!strcmp(pcField, "EGP")) {
    return ROUTE_ORIGIN_EGP;
  } else if (!strcmp(pcField, "INCOMPLETE")) {
    return ROUTE_ORIGIN_INCOMPLETE;
  }

  return 255;
}

// ----- mrtd_check_type --------------------------------------------
/**
 * This function checks that the given MRT protocol field is
 * compatible with the given MRT input type.
 *
 * For table dumps (MRTD_TYPE_RIB), the protocol field is expected to
 * contain 'TABLE_DUMP'.
 *
 * For messages (MRTD_TYPE_MSG), the protocol field is expected to
 * contain 'BGP' or 'BGP4'.
 */
int mrtd_check_type(char * pcField, mrtd_input_t tType)
{
  return (((tType == MRTD_TYPE_RIB) &&
	   !strcmp(pcField, "TABLE_DUMP")) ||
	  ((tType != MRTD_TYPE_RIB) &&
	   (!strcmp(pcField, "BGP") || !strcmp(pcField, "BGP4")))
	  );
}

// ----- mrtd_check_header ------------------------------------------
/**
 * Check an MRT route record's header. Return the MRT record type
 * which is one of 'A' (update message), 'W' (withdraw message) and
 * 'B' (best route).
 */
mrtd_input_t mrtd_check_header(STokens * pTokens)
{
  unsigned int uNumTokens, uReqTokens;
  mrtd_input_t tType;
  char * pcTemp;

  uNumTokens= tokens_get_num(pTokens);

  /* Check the number of fields (at least 5 for the "header") */
  if (uNumTokens < 5) {
    LOG_ERR(LOG_LEVEL_SEVERE,
	    "Error: not enough fields in MRT input (%d/5)\n", 
	    uNumTokens);
    return MRTD_TYPE_INVALID;
  }

  /* Check the type field. This field must contain
     - B for best routes in table dumps
     - A for updates in messages
     - W for withdraws in messages
  */
  pcTemp= tokens_get_string_at(pTokens, 2);
  if ((strlen(pcTemp) != 1) || (strchr("ABW", pcTemp[0]) == NULL)) {
    LOG_ERR(LOG_LEVEL_SEVERE,
	    "Error: invalid MRT record type field \"%s\"\n", pcTemp);
    return MRTD_TYPE_INVALID;
  }
  tType= pcTemp[0];

  /* Check the protocol field. This field must contain
     - TABLE_DUMP for table dumps
     - BGP / BGP4 for messages
  */
  pcTemp= tokens_get_string_at(pTokens, 0);
  if (!mrtd_check_type(pcTemp, tType)) {
    LOG_ERR(LOG_LEVEL_SEVERE,
	    "Error: incorrect MRT record protocol \"%s\"\n", pcTemp);
    return MRTD_TYPE_INVALID;
  }
  
  /* Check the timestamp field */
  if (tType == 'W')
    uReqTokens= 5;
  else
    uReqTokens= 11;

  if (uNumTokens < uReqTokens) {
    LOG_ERR(LOG_LEVEL_SEVERE,
	    "Error: not enough fields in MRT input (%d/%d)\n",
	    uNumTokens, uReqTokens);
    return MRTD_TYPE_INVALID;
  }

  return tType;
}

// ----- mrtd_create_route ------------------------------------------
/*
 * This function builds a route from the given set of tokens. The
 * function requires at least 11 tokens for a route that belongs to a
 * routing table dump or an update message (communities are
 * optional). The function requires 6 tokens for a withdraw message.
 * The set of tokens must be composed of the following fields (see the
 * MRT user's manual for more information):
 *
 * Token 0     -> Protocol
 *       1     -> Time (currently ignored)
 *       2     -> Type
 *       3     -> PeerIP
 *       4     -> PeerAS
 *       5     -> Prefix
 *       6     -> AS-Path
 *       7     -> Origin
 *       8     -> NextHop
 *       9     -> Local_Pref
 *       10    -> MED
 *       11    -> Community
 *       >= 12 -> currently ignored
 *
 * Parameters:
 *   - the router which is supposed to receive this route (or NULL)
 *   - the MRT record tokens
 *   - the destination prefix
 *   - a pointer to the resulting route
 *
 * Return values:
 *    0 in case of success (and ppRoute points to a valid BGP route)
 *   -1 in case of failure
 *
 * Note: the router field must be specified (i.e. be != NULL) when the
 * MRT record contains a BGP message. In this case, the peer
 * information (Peer IP and Peer AS) contained in the MRT record must
 * correspond to the router address/AS-number and the next-hops must
 * correspond to peers of the router.
 *
 * If no BGP router is specified, the route is considered as locally
 * originated.
 */
int mrtd_create_route(SBGPRouter * pRouter, SBGPPeer * pPeer,
		      STokens * pTokens,
		      SPrefix * pPrefix, SRoute ** ppRoute)
{
  mrtd_input_t tType;
  net_addr_t tPeerAddr;
  unsigned int uPeerAS;

  SRoute * pRoute;
  bgp_origin_t tOrigin;
  net_addr_t tNextHop;
  unsigned long ulLocalPref;
  unsigned long ulMed;
  char * pcTemp;
  char * pcEndPtr;
  SBGPPath * pPath= NULL;
  SCommunities * pComm= NULL;

  /* No route built until now */
  *ppRoute= NULL;

  /* Check header, length and get type (route/update/withdraw) */
  tType= mrtd_check_header(pTokens);
  if (tType == MRTD_TYPE_INVALID) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: invalid headers\n");
    return -1;
  }

  /* Check the PeerIP field */
  pcTemp= tokens_get_string_at(pTokens, 3);
  if (ip_string_to_address(pcTemp, &pcEndPtr, &tPeerAddr) ||
      (*pcEndPtr != '\0')) {
    LOG_ERR(LOG_LEVEL_SEVERE,
	    "Error: invalid PeerIP field \"%s\"\n", pcTemp);
    return -1;
  }  
  
  /* Check the PeerAS field */
  if (tokens_get_uint_at(pTokens, 4, &uPeerAS) ||
      (uPeerAS > 65535)) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: invalid PeerAS field \"%s\"\n",
	       tokens_get_string_at(pTokens, 4));
    return -1;
  }

  if (pRouter != NULL) {

    /* Check that the peer fields correspond to the local router */
    if ((pRouter->pNode->tAddr != tPeerAddr) ||
	(pRouter->uNumber != uPeerAS)) {
      
      LOG_ERR_ENABLED(LOG_LEVEL_SEVERE) {
	log_printf(pLogErr,
		   "Error: invalid peer (IP address/AS number mismatch)\n"
		   "Error: local router = AS%d:", pRouter->uNumber);
	ip_address_dump(pLogErr, pRouter->pNode->tAddr);
	log_printf(pLogErr,"\nError: MRT peer router = AS%d:", uPeerAS);
	ip_address_dump(pLogErr, tPeerAddr);
	log_printf(pLogErr, "\n");
      }
      return -1;

    }

  }
  
  /* Check the prefix */
  pcTemp= tokens_get_string_at(pTokens, 5);
  if (ip_string_to_prefix(pcTemp, &pcEndPtr, pPrefix) ||
      (*pcEndPtr != 0)) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: not a valid prefix \"%s\"\n", pcTemp);
    return -1;
  }

  if (tType == MRTD_TYPE_WITHDRAW)
    return tType;

  /* Check the AS-PATH */
  pcTemp= tokens_get_string_at(pTokens, 6);
  pPath= mrtd_create_path(pcTemp);
  if (pPath == NULL) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: not a valid AS-Path \"%s\"\n", pcTemp);
    return -1;
  }

  /* Check ORIGIN (currently, the origin is always IGP) */
  pcTemp= tokens_get_string_at(pTokens, 7);
  tOrigin= mrtd_get_origin(pcTemp);
  if (tOrigin == 255) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: not a valid origin \"%s\"\n", pcTemp);
    return -1;
  }

  /* Check the NEXT-HOP */
  pcTemp= tokens_get_string_at(pTokens, 8);
  if (ip_string_to_address(pcTemp, &pcEndPtr, &tNextHop) ||
      (*pcEndPtr != 0)) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: not a valid next-hop \"%s\"\n", pcTemp);
    path_destroy(&pPath);
    return -1;
  }

  /* If a router is specified, but no peer, the next-hop field is the
   * sole information we have. Thus, we must be able to find in the
   * router a peer that corresponds to that next-hop. */
  if ((pRouter != NULL) && (pPeer == NULL)) {

    // Look for the peer in the router...
    pPeer= bgp_router_find_peer(pRouter, tNextHop);
    /* If the peer does not exist, auto-create it if required or
       drop an error message. */
    if (pPeer == NULL) {
      if (!BGP_OPTIONS_AUTO_CREATE) {
	LOG_ERR_ENABLED(LOG_LEVEL_SEVERE) {
	  log_printf(pLogErr, "Error: peer not found \"");
	  ip_address_dump(pLogErr, tNextHop);
	  log_printf(pLogErr, "\"\n");
	}
	path_destroy(&pPath);
	return -1;
      } else {
	bgp_auto_config_session(pRouter, tNextHop,
				path_last_as(pPath), &pPeer);
      }
    }
  }

  /* Check the LOCAL-PREF */
  if (tokens_get_ulong_at(pTokens, 9, &ulLocalPref)) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: not a valid local-preference \"%s\"\n",
	       tokens_get_string_at(pTokens, 9));
    path_destroy(&pPath);
    return -1;
  }

  /* Check the MED */
  if (tokens_get_ulong_at(pTokens, 10, &ulMed)) {
    LOG_ERR(LOG_LEVEL_SEVERE,
	    "Error: not a valid multi-exit-discriminator \"%s\"\n",
	    tokens_get_string_at(pTokens, 10));
    path_destroy(&pPath);
    return -1;
  }

  /* Check the COMMUNITIES (if present) */
  if (tokens_get_num(pTokens) > 11) {
    pcTemp= tokens_get_string_at(pTokens, 11);
    pComm= mrtd_create_communities(pcTemp);
    if (pComm == NULL) {
      LOG_ERR(LOG_LEVEL_SEVERE, "Error: invalid communities \"%s\"\n", pcTemp);
      path_destroy(&pPath);
      return -1;
    }
  }

  /* Build route */
  pRoute= route_create(*pPrefix, NULL, tNextHop, tOrigin);
  route_localpref_set(pRoute, ulLocalPref);
  route_med_set(pRoute, ulMed);
  route_set_path(pRoute, pPath);
  route_set_comm(pRoute, pComm);
  pRoute->pPeer= pPeer;
  if (pRoute->pPeer == NULL)
    route_flag_set(pRoute, ROUTE_FLAG_INTERNAL, 1);

  *ppRoute= pRoute;

  LOG_DEBUG(LOG_LEVEL_DEBUG, "ROUTE CREATED FROM MRT RECORD :-)\n");

  return tType;
}

// ----- mrtd_route_from_line ---------------------------------------
/**
 * This function parses a string that contains an MRT record. Based on
 * the record's content, a BGP route is created. For more information
 * on the MRT record format, see 'mrtd_create_route'.
 *
 * Parameters:
 *
 */
SRoute * mrtd_route_from_line(SBGPRouter * pRouter, char * pcLine)
{
  STokens * pTokens;
  SRoute * pRoute;
  SPrefix sPrefix;

  if (pLineTokenizer == NULL)
    pLineTokenizer= tokenizer_create("|", 1, NULL, NULL);

  /* Really parse the line... */
  if (tokenizer_run(pLineTokenizer, pcLine) != TOKENIZER_SUCCESS) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: could not parse line in MRTD RIB\n");
    return NULL;
  }
      
  pTokens= tokenizer_get_tokens(pLineTokenizer);
  
  if (mrtd_create_route(pRouter, NULL, pTokens, &sPrefix, &pRoute)
      == MRTD_TYPE_RIB) {
    return pRoute;
  } else {
    LOG_ERR(LOG_LEVEL_SEVERE,
	    "Error: could not build route from line:\n\"%s\"\n", pcLine);
  }

  if (pRoute != NULL)
    route_destroy(&pRoute);
  
  return NULL;
}

// ----- mrtd_msg_from_line -----------------------------------------
/**
 * This function parses a string that contains an MRT record. Based on
 * the record's content, a BGP Update or Withdraw message is built.
 *
 * Parameters:
 *   - the router which is supposed to receive the message (or NULL)
 *   - the peer which is supposed to send the message
 *   - the strings that contains the MRT BGP message
 *
 * Note: in case a router has been specified (i.e. pRouter != NULL),
 * the function checks that the message comes from a valid peer of the
 * router.
 */
SBGPMsg * mrtd_msg_from_line(SBGPRouter * pRouter, SBGPPeer * pPeer,
			     char * pcLine)
{
  SBGPMsg * pMsg= NULL;
  STokens * pTokens;
  SRoute * pRoute;
  SPrefix sPrefix;
#if defined __EXPERIMENTAL__ && __EXPERIMENTAL_WALTON__
  net_addr_t tNextHop;
#endif

  if (pLineTokenizer == NULL)
    pLineTokenizer= tokenizer_create("|", 1, NULL, NULL);

  /* Really parse the line... */
  if (tokenizer_run(pLineTokenizer, pcLine) != TOKENIZER_SUCCESS) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: could not parse line in MRTD RIB\n");
    return NULL;
  }
      
  pTokens= tokenizer_get_tokens(pLineTokenizer);
  
  switch (mrtd_create_route(pRouter, pPeer, pTokens, &sPrefix, &pRoute)) {
  case MRTD_TYPE_UPDATE:
    pMsg= bgp_msg_update_create(pPeer->uRemoteAS, pRoute);
    break;
  case MRTD_TYPE_WITHDRAW:
#if defined __EXPERIMENTAL__ && __EXPERIMENTAL_WALTON__
    //TODO: Is it possible to change the MRT format to adapt to Walton ?
    tNextHop = route_get_nexthop(pRoute);
    pMsg= bgp_msg_withdraw_create(pPeer->tAddr, sPrefix, &tNextHop);
#else
    pMsg= bgp_msg_withdraw_create(pPeer->tAddr, sPrefix);
#endif
    break;
  default:
    route_destroy(&pRoute);
  }
  
  return pMsg;
}

// ----- mrtd_ascii_load_routes -------------------------------------
/**
 * This function loads all the routes from a table dump in MRT
 * format. The filename must have previously been converted to ASCII
 * using 'route_btoa -m'.
 */
SPtrArray * mrtd_ascii_load_routes(SBGPRouter * pRouter, char * pcFileName)
{
  FILE * pFile;
  SRoutes * pRoutes= NULL;
  unsigned int uLineNumber= 0;
  char acFileLine[1024];
  SRoute * pRoute;
  int iError= 0;
  int iIndex;

  pFile= fopen(pcFileName, "r");
  if (pFile != NULL) {

    pRoutes= routes_list_create(ROUTES_LIST_OPTION_REF);

    while ((!feof(pFile)) && (!iError)) {
      if (fgets(acFileLine, sizeof(acFileLine), pFile) == NULL)
	break;
      uLineNumber++;

      /* Create a route from the file line */
      pRoute= mrtd_route_from_line(pRouter, acFileLine);

      /* In case of error, the MRT record is ignored */
      if (pRoute == NULL) {
	LOG_ERR(LOG_LEVEL_SEVERE,
		"Warning: could not load the MRT record at line %d\n",
		uLineNumber);
      } else
	routes_list_append(pRoutes, pRoute);
      
    }

    fclose(pFile);
  }
  if (iError) {
    /* In case of any error, all the routes in the list MUST BE FREED
       (since the list is only a list of references) */
    for (iIndex= 0; iIndex < routes_list_get_num(pRoutes); iIndex++)
      route_destroy((SRoute **) &pRoutes->data[iIndex]);
    routes_list_destroy(&pRoutes);
    pRoutes= NULL;
  }

  LOG_DEBUG(LOG_LEVEL_DEBUG, "ROUTES LOADED :-)\n");

  return pRoutes;
}

/////////////////////////////////////////////////////////////////////
//
// BINARY MRT (based on libbgpdump-1.4)
//
/////////////////////////////////////////////////////////////////////

// -----[ mrtd_process_community ]-----------------------------------
/**
 *
 */
#ifdef HAVE_BGPDUMP
SCommunities * mrtd_process_community(struct community * com)
{
  SCommunities * pCommunities= NULL;
  int iIndex;
  uint32_t comval;

  for (iIndex = 0; iIndex < com->size; iIndex++) {

    if (pCommunities == NULL)
      pCommunities= comm_create();
    
    memcpy(&comval, com_nthval(com, iIndex), sizeof (uint32_t));
    comm_add(&pCommunities, ntohl(comval));
    
  }

  return pCommunities;
}
#endif

// -----[ mrtd_process_aspath ]--------------------------------------
/**
 * Convert a bgpdump aspath to a C-BGP as-path. Note: this function is
 * based on Dan Ardelean's code to convert an aspath to a string.
 *
 * This function only supports SETs and SEQUENCEs. If such a segment
 * is found, the function will fail (and return NULL).
 */
#ifdef HAVE_BGPDUMP
SBGPPath * mrtd_process_aspath(struct aspath * path)
{
  SBGPPath * pPath= NULL;
  SPathSegment * pSegment;
  void * pnt;
  void * end;
  struct assegment *assegment;
  int iIndex;
  int iResult= 0;

  /* empty AS-path */
  if (path->length == 0)
    return NULL;

  /* Set initial pointers */
  pnt= path->data;
  end= pnt + path->length;

  while (pnt < end) {
    
    /* Process next segment... */
    assegment= (struct assegment *) pnt;
    
    /* Get the AS-path segment type. */
    if (assegment->type == AS_SET) {
      pSegment= path_segment_create(AS_PATH_SEGMENT_SET,
				    assegment->length);
    } else if (assegment->type == AS_SEQUENCE) {
      pSegment= path_segment_create(AS_PATH_SEGMENT_SEQUENCE,
				    assegment->length);
    } else {
      iResult= -1;
      break;
    }
    
    /* Check the AS-path segment length. */
    if ((pnt + (assegment->length * AS_VALUE_SIZE) + AS_HEADER_SIZE) > end) {
      iResult= -1;
      break;
    }

    /* Copy each AS number into the AS-path segment */
    for (iIndex= 0; iIndex < assegment->length; iIndex++)
      pSegment->auValue[assegment->length-iIndex-1]=
	ntohs(assegment->asval[iIndex]);

    /* Add the segment to the AS-path */
    if (pPath == NULL)
      pPath= path_create();
    path_add_segment(pPath, pSegment);

    /* Go to next segment... */
    pnt+= (assegment->length * AS_VALUE_SIZE) + AS_HEADER_SIZE;
  }

  /* If something went wrong, clean current AS-path */
  if ((iResult != 0) && (pPath != NULL)) {
    path_destroy(&pPath);
    pPath= NULL;
  }

  return pPath;
}
#endif

// -----[ mrtd_process_table_dump ]----------------------------------
/**
 * Convert an MRT TABLE DUMP to a C-BGP route.
 *
 * This function currently only supports IPv4 address familly. In
 * addition, the function only retrive the following attributes:
 * next-hop, origin, local-pref, med, as-path
 *
 * todo: conversion of communities
 */
#ifdef HAVE_BGPDUMP
SRoute * mrtd_process_table_dump(BGPDUMP_ENTRY * pEntry)
{
  BGPDUMP_MRTD_TABLE_DUMP * pTableDump= &pEntry->body.mrtd_table_dump;
  SBGPPeer * pPeer;
  SRoute * pRoute= NULL;
  SPrefix sPrefix;
  bgp_origin_t tOrigin;
  net_addr_t tNextHop;

  if (pEntry->subtype == AFI_IP) {

    pPeer= bgp_peer_create(pTableDump->peer_as,
			   ntohl(pTableDump->peer_ip.v4_addr.s_addr),
			   NULL, 0);

    sPrefix.tNetwork= ntohl(pTableDump->prefix.v4_addr.s_addr);
    sPrefix.uMaskLen= pTableDump->mask;
    if (pEntry->attr == NULL)
      return NULL;

    if ((pEntry->attr->flag & ATTR_FLAG_BIT(BGP_ATTR_ORIGIN)) != 0)
      tOrigin= (bgp_origin_t) pEntry->attr->origin;
    else
      return NULL;

    if ((pEntry->attr->flag & ATTR_FLAG_BIT(BGP_ATTR_NEXT_HOP)) != 0)
      tNextHop= ntohl(pEntry->attr->nexthop.s_addr);
    else
      return NULL;

    pRoute= route_create(sPrefix, pPeer, tNextHop, tOrigin);

    if ((pEntry->attr->flag & ATTR_FLAG_BIT(BGP_ATTR_AS_PATH)) != 0)
      route_set_path(pRoute, mrtd_process_aspath(pEntry->attr->aspath));

    if ((pEntry->attr->flag & ATTR_FLAG_BIT(BGP_ATTR_LOCAL_PREF)) != 0)
      route_localpref_set(pRoute, pEntry->attr->local_pref);

    if ((pEntry->attr->flag & ATTR_FLAG_BIT(BGP_ATTR_MULTI_EXIT_DISC)) != 0)
      route_med_set(pRoute, pEntry->attr->med);

    if ((pEntry->attr->flag & ATTR_FLAG_BIT(BGP_ATTR_COMMUNITIES)) != 0)
      pRoute->pAttr->pCommunities=
	mrtd_process_community(pEntry->attr->community);

  }

  return pRoute;
}
#endif

// -----[ mrtd_process_entry ]---------------------------------------
/**
 * Convert a bgpdump entry to a route. Only supports entries of type
 * TABLE DUMP, for adress family AF_IP (IPv4).
 */
#ifdef HAVE_BGPDUMP
SRoute * mrtd_process_entry(BGPDUMP_ENTRY * pEntry)
{
  SRoute * pRoute= NULL;

  if (pEntry->type == BGPDUMP_TYPE_MRTD_TABLE_DUMP) {
      pRoute= mrtd_process_table_dump(pEntry);
  }

  return pRoute;
}
#endif

// -----[ mrtd_load_routes ]-----------------------------------------
/**
 *
 */
#ifdef HAVE_BGPDUMP
SRoutes * mrtd_load_routes(const char * pcFileName, int iOnlyDump,
			   SFilterMatcher * pMatcher)
{
  BGPDUMP * fDump;
  BGPDUMP_ENTRY * pDumpEntry= NULL;
  SRoute * pRoute;
  SRoutes * pRoutes= NULL;

  if ((fDump= bgpdump_open_dump(pcFileName)) == NULL)
    return NULL;

  do {
    pDumpEntry= bgpdump_read_next(fDump);
    if (pDumpEntry != NULL) {
      pRoute= mrtd_process_entry(pDumpEntry);
      if (pRoute != NULL) {
	if (iOnlyDump) {
	  if ((pMatcher == NULL) ||
	      filter_matcher_apply(pMatcher, NULL, pRoute)) {
	    route_dump(pLogOut, pRoute);
	    log_printf(pLogOut, "\n");
	  }
	  route_destroy(&pRoute);
	} else {
	  if (pRoutes == NULL)
	    pRoutes= routes_list_create(0);
	  routes_list_append(pRoutes, pRoute);
	}
      }
      bgpdump_free_mem(pDumpEntry);
    } else {
      LOG_ERR(LOG_LEVEL_SEVERE, "Error reading MRT dump entry\n");
      if (pRoutes != NULL)
	routes_list_destroy(&pRoutes);
      break;
    }
  } while(fDump->eof == 0);
  
  bgpdump_close_dump(fDump);

  return pRoutes;
}
#endif

/////////////////////////////////////////////////////////////////////
// INITIALIZATION AND FINALIZATION SECTION
/////////////////////////////////////////////////////////////////////

// ----- _mrtd_destroy ----------------------------------------------
/**
 *
 */
void _mrtd_destroy()
{
  tokenizer_destroy(&pLineTokenizer);
  tokenizer_destroy(&pCommTokenizer);
  tokenizer_destroy(&pPathTokenizer);
  tokenizer_destroy(&pSegmentTokenizer);
}
