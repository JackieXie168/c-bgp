// ==================================================================
// @(#)mrtd.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 20/02/2004
// @lastdate 27/01/2005
// ==================================================================
// Current limitations:
// - origin is always IGP
// Future changes:
// - move attribute parsers in corresponding sections

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <string.h>

#include <libgds/tokenizer.h>
#include <libgds/tokens.h>

#include <bgp/as_t.h>
#include <bgp/as.h>
#include <bgp/peer.h>
#include <bgp/comm.h>
#include <bgp/mrtd.h>
#include <bgp/path.h>
#include <bgp/path_segment.h>
#include <bgp/route.h>
#include <bgp/routes_list.h>
#include <net/prefix.h>

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
    LOG_SEVERE("Error: parse error in 'mrtd_create_path_segment'\n");
    return NULL;
  }

  pTokens= tokenizer_get_tokens(pSegmentTokenizer);
  pSegment= path_segment_create(AS_PATH_SEGMENT_SET, 0);
  for (iIndex= tokens_get_num(pTokens); iIndex > 0; iIndex--) {
    if (tokens_get_ulong_at(pTokens, iIndex-1, &ulASNum) || (ulASNum > 65535)) {
      LOG_SEVERE("Error: invalid AS-Num \"%s\"\n",
		 tokens_get_string_at(pTokens, iIndex-1));
      path_segment_destroy(&pSegment);
      break;
    }
    path_segment_add(&pSegment, ulASNum);
  }
  return pSegment;
}

// ----- mrtd_create_path -------------------------------------------
SPath * mrtd_create_path(char * pcPath)
{
  int iIndex;
  STokens * pPathTokens;
  SPath * pPath= NULL;
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
	LOG_SEVERE("Error: not a valid AS-Num \"%s\"\n",
		   pcSegment);
	path_destroy(&pPath);
	break;
      }
      path_append(&pPath, ulASNum);
    } else {
      pSegment= mrtd_create_path_segment(pcSegment);
      if (pSegment == NULL) {
	LOG_SEVERE("Error: not a valid path segment \"%s\"\n",
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
      LOG_SEVERE("Error: not a valid community \"%s\"\n",
		 tokens_get_string_at(pTokens, iIndex));
      comm_destroy(&pComm);
      return NULL;
    }
    comm_add(pComm, uComm);
  }
  
  return pComm;
}

// ----- mrtd_get_origin --------------------------------------------
/**
 * This function converts the origin field of an MRT record to the
 * route origin. If the route origin is unknown, -1 is returned.
 */
int mrtd_get_origin(char * pcField)
{
  if (!strcmp(pcField, "IGP")) {
    return ROUTE_ORIGIN_IGP;
  } else if (!strcmp(pcField, "EGP")) {
    return ROUTE_ORIGIN_EGP;
  } else if (!strcmp(pcField, "INCOMPLETE")) {
    return ROUTE_ORIGIN_INCOMPLETE;
  }

  return -1;
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
 *
 */
mrtd_input_t mrtd_check_header(STokens * pTokens)
{
  unsigned int uNumTokens, uReqTokens;
  mrtd_input_t tType;
  char * pcTemp;

  uNumTokens= tokens_get_num(pTokens);

  /* Check the number of fields (at least 5 for the "header") */
  if (uNumTokens < 5) {
    LOG_SEVERE("Error: not enough fields in MRT input (%d)\n", 
	       uNumTokens);
    return -1;
  }

  /* Check the type field. This field must contain
     - B for best routes in table dumps
     - A for updates in messages
     - W for withdraws in messages
  */
  pcTemp= tokens_get_string_at(pTokens, 2);
  if (strlen(pcTemp) != 1) {
    LOG_SEVERE("Error: invalid MRT record type field \"%s\"\n", pcTemp);
    return -1;
  }
  tType= pcTemp[0];

  /* Check the protocol field. This field must contain
     - TABLE_DUMP for table dumps
     - BGP / BGP4 for messages
  */
  pcTemp= tokens_get_string_at(pTokens, 0);
  if (!mrtd_check_type(pcTemp, tType)) {
    LOG_SEVERE("Error: incorrect MRT record protocol \"%s\"\n", pcTemp);
    return -1;
  }
  
  /* Check the timestamp field */
  if (tType == 'W')
    uReqTokens= 5;
  else
    uReqTokens= 11;

  if (uNumTokens < uReqTokens) {
    LOG_SEVERE("Error: not enough fields in MRT input\n");
    return -1;
  }

  return tType;
}

// ----- mrtd_create_route ------------------------------------------
/*
 * This function builds a route from the given set of tokens. The
 * function requires at least 11 tokens for a route that belongs to a
 * routing table dump or an update message (communities are
 * optional). The function requires 6 tokens for a withdraw message.
 *
 * If a BGP router is given (pRouter), then the peer information
 * contained in the MRT record must correspond to the router
 * address/AS-number and the next-hops must correspond to peers of the
 * router.
 *
 * If no BGP router is specified, the route is considered as locally
 * originated.
 */
int mrtd_create_route(SBGPRouter * pRouter, STokens * pTokens,
		      SPrefix * pPrefix, SRoute ** ppRoute)
{
  int iType;
  net_addr_t tPeerAddr;
  unsigned int uPeerAS;
  SPeer * pPeer= NULL;

  SRoute * pRoute;
  origin_type_t tOrigin;
  net_addr_t tNextHop;
  unsigned long ulLocalPref;
  char * pcTemp;
  char * pcEndPtr;
  SPath * pPath= NULL;
  SCommunities * pComm= NULL;

  /* No route built until now */
  *ppRoute= NULL;

  /* Check header, length and get type (route/update/withdraw) */
  iType= mrtd_check_header(pTokens);
  if (iType < 0)
    return -1;

  /* Check the PeerIP field */
  pcTemp= tokens_get_string_at(pTokens, 3);
  if (ip_string_to_address(pcTemp, &pcEndPtr, &tPeerAddr) ||
      (*pcEndPtr != '\0')) {
    LOG_SEVERE("Error: invalid PeerIP field \"%s\"\n", pcTemp);
    return -1;
  }  
  
  /* Check the PeerAS field */
  if (tokens_get_uint_at(pTokens, 4, &uPeerAS) ||
      (uPeerAS > 65535)) {
    LOG_SEVERE("Error: invalid PeerAS field \"%s\"\n",
	       tokens_get_string_at(pTokens, 4));
    return -1;
  }

  if (pRouter != NULL) {

    /* Check that the peer fields correspond to the local router */
    if ((pRouter->pNode->tAddr != tPeerAddr) ||
	(pRouter->uNumber != uPeerAS)) {
      
      LOG_SEVERE("Error: invalid peer (IP address/AS number mismatch)\n");
      LOG_SEVERE("Error: local router = AS%d:", pRouter->uNumber);
      ip_address_dump(log_get_stream(pMainLog), pRouter->pNode->tAddr);
      LOG_SEVERE("\n");
      LOG_SEVERE("Error: MRT peer router = AS%d:", uPeerAS);
      ip_address_dump(log_get_stream(pMainLog), tPeerAddr);
      LOG_SEVERE("\n");
      return -1;

    }

  }
  
  /* Check the prefix */
  if (ip_string_to_prefix(tokens_get_string_at(pTokens, 5),
			  &pcEndPtr, pPrefix) ||
      (*pcEndPtr != 0)) {
    LOG_SEVERE("Error: not a valid prefix \"%s\"\n",
	       tokens_get_string_at(pTokens, 5));
    return -1;
  }

  if (iType == MRTD_TYPE_WITHDRAW)
    return iType;

  /* Check the AS-PATH */
  pPath= mrtd_create_path(tokens_get_string_at(pTokens, 6));
  if (pPath == NULL) {
    LOG_SEVERE("Error: not a valid AS-Path \"%s\"\n",
	       tokens_get_string_at(pTokens, 6));
    return -1;
  }

  /* Check ORIGIN (currently, the origin is always IGP) */
  tOrigin= ROUTE_ORIGIN_IGP;

  /* Check the NEXT-HOP */
  if (ip_string_to_address(tokens_get_string_at(pTokens, 8),
			   &pcEndPtr, &tNextHop) ||
      (*pcEndPtr != 0)) {
    LOG_SEVERE("Error: not a valid next-hop \"%s\"\n",
	       tokens_get_string_at(pTokens, 8));
    path_destroy(&pPath);
    return -1;
  }

  if ((pRouter != NULL) && (pRouter->pNode->tAddr != tNextHop)) {
    
    /* Look for the peer in the router... */
    pPeer= as_find_peer(pRouter, tNextHop);
    if (pPeer == NULL) {
      LOG_SEVERE("Error: peer not found \"");
      LOG_ENABLED_SEVERE()
	ip_address_dump(log_get_stream(pMainLog), tNextHop);
      LOG_SEVERE("\"\n");
      return -1;
    }
      
  }

  /* Check the LOCAL-PREF */
  if (tokens_get_ulong_at(pTokens, 9, &ulLocalPref)) {
    LOG_SEVERE("Error: not a valid local-preference \"%s\"\n",
	       tokens_get_string_at(pTokens, 9));
    path_destroy(&pPath);
    return -1;
  }

  /* Check the MED */

  /* Check COMMUNITIES, if present */
  if (tokens_get_num(pTokens) > 11) {
    pComm= mrtd_create_communities(tokens_get_string_at(pTokens, 11));
    if (pComm == NULL) {
      LOG_SEVERE("Error: invalid communities \"%s\"\n",
		 tokens_get_string_at(pTokens, 11));
      path_destroy(&pPath);
      return -1;
    }

  }

  /* Build route */
  pRoute= route_create(*pPrefix, NULL, tNextHop, tOrigin);
  route_localpref_set(pRoute, ulLocalPref);
  pRoute->pASPath= pPath;
  pRoute->pCommunities= pComm;
  pRoute->pPeer= pPeer;
  if (pRoute->pPeer == NULL)
    route_flag_set(pRoute, ROUTE_FLAG_INTERNAL, 1);

  *ppRoute= pRoute;

  return iType;
}

// ----- mrtd_route_from_line ---------------------------------------
/**
 * This function converts the given line into a route. The line must
 * be in MRT format. The line must be composed of the following fields
 * (see the MRT user's manual for more information):
 *
 * Protocol | Time | Type | PeerIP | PeerAS | Prefix | ASPATH |
 *   Origin | NextHop | Local_Pref | MED | Community
 *
 * Note: additional fields are ignored.
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
    LOG_SEVERE("Error: could not parse line in MRTD RIB\n");
    return NULL;
  }
      
  pTokens= tokenizer_get_tokens(pLineTokenizer);
  
  if (mrtd_create_route(pRouter, pTokens, &sPrefix, &pRoute) == MRTD_TYPE_RIB)
    return pRoute;

  route_destroy(&pRoute);

  return NULL;
}

// ----- mrtd_msg_from_line -----------------------------------------
/**
 *
 */
SBGPMsg * mrtd_msg_from_line(SBGPRouter * pRouter, SPeer * pPeer,
			     char * pcLine)
{
  SBGPMsg * pMsg= NULL;
  STokens * pTokens;
  SRoute * pRoute;
  SPrefix sPrefix;

  if (pLineTokenizer == NULL)
    pLineTokenizer= tokenizer_create("|", 1, NULL, NULL);

  /* Really parse the line... */
  if (tokenizer_run(pLineTokenizer, pcLine) != TOKENIZER_SUCCESS) {
    LOG_SEVERE("Error: could not parse line in MRTD RIB\n");
    return NULL;
  }
      
  pTokens= tokenizer_get_tokens(pLineTokenizer);
  
  switch (mrtd_create_route(pRouter, pTokens, &sPrefix, &pRoute)) {
  case MRTD_TYPE_UPDATE:
    if (pRoute->pPeer != pPeer)
      break;
    pMsg= bgp_msg_update_create(pPeer->tAddr, pRoute);
    break;
  case MRTD_TYPE_WITHDRAW:
    pMsg= bgp_msg_withdraw_create(pPeer->tAddr, sPrefix);
    break;
  default:
    route_destroy(&pRoute);
  }
  
  return pMsg;
}

// ----- mrtd_load_routes -------------------------------------------
/**
 * This function loads all the routes from a table dump in MRT
 * format.
 */
SPtrArray * mrtd_load_routes(SBGPRouter * pRouter, char * pcFileName)
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

    pRoutes= routes_list_create();

    while ((!feof(pFile)) && (!iError)) {
      if (fgets(acFileLine, sizeof(acFileLine), pFile) == NULL)
	break;
      uLineNumber++;

      /* Create a route from the file line */
      pRoute= mrtd_route_from_line(pRouter, acFileLine);

      /* In case of error, the MRT record is ignored */
      if (pRoute == NULL) {
	LOG_SEVERE("Warning: could not load the MRT record at line %d\n",
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
  return pRoutes;
}

/////////////////////////////////////////////////////////////////////
// INITIALIZATION AND FINALIZATION SECTION
/////////////////////////////////////////////////////////////////////

void _mrtd_destroy() __attribute__((destructor));

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
