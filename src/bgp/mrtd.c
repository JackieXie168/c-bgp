// ==================================================================
// @(#)mrtd.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 20/02/2004
// @lastdate 23/02/2004
// ==================================================================
// Current limitations:
// - origin is always IGP
// Future changes:
// - move attribute parsers in corresponding sections

#include <stdio.h>
#include <string.h>

#include <libgds/tokenizer.h>
#include <libgds/tokens.h>

#include <bgp/as_t.h>
#include <bgp/comm.h>
#include <bgp/mrtd.h>
#include <bgp/path.h>
#include <bgp/path_segment.h>
#include <bgp/route.h>
#include <bgp/routes_list.h>
#include <net/prefix.h>

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
    pSegmentTokenizer= tokenizer_create(" ", NULL, NULL);

  if (tokenizer_run(pSegmentTokenizer, pcPathSegment) != TOKENIZER_SUCCESS) {
    LOG_SEVERE("Error: parse error in 'mrtd_create_path_segment'\n");
    return NULL;
  }

  pTokens= tokenizer_get_tokens(pSegmentTokenizer);
  pSegment= path_segment_create(AS_PATH_SEGMENT_SET, 0);
  for (iIndex= 0; iIndex < tokens_get_num(pTokens); iIndex++) {
    if (tokens_get_ulong_at(pTokens, iIndex, &ulASNum) || (ulASNum > 65535)) {
      LOG_SEVERE("Error: invalid AS-Num \"%s\"\n",
		 tokens_get_string_at(pTokens, iIndex));
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
    pPathTokenizer= tokenizer_create(" ", "[", "]");

  if (tokenizer_run(pPathTokenizer, pcPath) != TOKENIZER_SUCCESS)
    return NULL;

  pPathTokens= tokenizer_get_tokens(pPathTokenizer);

  pPath= path_create();
  for (iIndex= 0; iIndex < tokens_get_num(pPathTokens); iIndex++) {
    pcSegment= tokens_get_string_at(pPathTokens, iIndex);
    if (!tokens_get_ulong_at(pPathTokens, iIndex, &ulASNum)) {
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
    pCommTokenizer= tokenizer_create(" ", NULL, NULL);

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

// ----- mrtd_create_route ------------------------------------------
SRoute * mrtd_create_route(STokens * pTokens)
{
  SRoute * pRoute;
  SPrefix sPrefix;
  origin_type_t tOrigin;
  net_addr_t tNextHop;
  unsigned long ulLocalPref;
  char * pcEndChar;
  SPath * pPath= NULL;
  SCommunities * pComm= NULL;
  unsigned int uNumTokens;

  uNumTokens= tokens_get_num(pTokens);

  // Check number of fields
  if (uNumTokens < 11) {
    LOG_SEVERE("Error: not enough fields in MRTD RIB (%d)\n",
	       tokens_get_num(pTokens));
    return NULL;
  }

  // Check 'TABLE_DUMP'
  if (strcmp(tokens_get_string_at(pTokens, 0), "TABLE_DUMP")) {
    LOG_SEVERE("Error: not a TABLE_DUMP record, header is \"%s\"\n",
	       tokens_get_string_at(pTokens, 0));
    return NULL;
  }
  
  // Check prefix
  if (ip_string_to_prefix(tokens_get_string_at(pTokens, 5),
			  &pcEndChar,
			  &sPrefix) || (*pcEndChar != 0)) {
    LOG_SEVERE("Error: not a valid prefix \"%s\"\n",
	       tokens_get_string_at(pTokens, 5));
    return NULL;
  }

  // Check AS-PATH
  pPath= mrtd_create_path(tokens_get_string_at(pTokens, 6));
  if (pPath == NULL) {
    LOG_SEVERE("Error: not a valid AS-Path \"%s\"\n",
	       tokens_get_string_at(pTokens, 6));
    return NULL;
  }

  // Check ORIGIN
  tOrigin= ROUTE_ORIGIN_IGP;

  // Check NEXT-HOP
  if (ip_string_to_address(tokens_get_string_at(pTokens, 8),
			   &pcEndChar,
			   &tNextHop) || (*pcEndChar != 0)) {
    LOG_SEVERE("Error: not a valid next-hop \"%s\"\n",
	       tokens_get_string_at(pTokens, 8));
    path_destroy(&pPath);
    return NULL;
  }

  // Check LOCAL-PREF
  if (tokens_get_ulong_at(pTokens, 9, &ulLocalPref)) {
    LOG_SEVERE("Error: not a valid local-preference \"%s\"\n",
	       tokens_get_string_at(pTokens, 9));
    path_destroy(&pPath);
    return NULL;
  }

  // Check COMMUNITIES
  if (uNumTokens > 11) {
    pComm= mrtd_create_communities(tokens_get_string_at(pTokens, 11));
    if (pComm == NULL) {
      LOG_SEVERE("Error: not valid communities \"%s\"\n",
		 tokens_get_string_at(pTokens, 11));
      path_destroy(&pPath);
      return NULL;
    }

  }
  
  pRoute= route_create(sPrefix, NULL, tNextHop, tOrigin);
  route_localpref_set(pRoute, ulLocalPref);
  pRoute->pASPath= pPath;
  pRoute->pCommunities= pComm;

  return pRoute;
}

// ----- mrtd_load_routes -------------------------------------------
/**
 *
 */
SPtrArray * mrtd_load_routes(char * pcFileName)
{
  FILE * pFile;
  STokenizer * pTokenizer;
  SRoutes * pRoutes= NULL;
  unsigned int uLineNumber= 0;
  char acFileLine[1024];
  STokens * pTokens;
  SRoute * pRoute;
  int iError= 0;
  int iIndex;

  pFile= fopen(pcFileName, "r");
  if (pFile != NULL) {

    pTokenizer= tokenizer_create("|", NULL, NULL);

    pRoutes= routes_list_create();

    while ((!feof(pFile)) && (!iError)) {
      if (fgets(acFileLine, sizeof(acFileLine), pFile) == NULL)
	break;
      uLineNumber++;

      if (tokenizer_run(pTokenizer, acFileLine) != TOKENIZER_SUCCESS) {
	LOG_SEVERE("Error: could not parse line in MRTD RIB\n");
	LOG_SEVERE("Error: \"%s\"\n", acFileLine);
	iError= -1;
	break;
      }
      
      pTokens= tokenizer_get_tokens(pTokenizer);

      pRoute= mrtd_create_route(pTokens);
      if (pRoute == NULL) {
	LOG_SEVERE("Error: syntax error, \"%s\", line %d\n",
		   pcFileName, uLineNumber);
	iError= -1;
	break;
      } else
	routes_list_append(pRoutes, pRoute);
      
    }

    tokenizer_destroy(&pTokenizer);

    fclose(pFile);
  }
  if (iError) {
    // MUST FREE all routes in the list (since it is only a list of
    // references)
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
  tokenizer_destroy(&pCommTokenizer);
  tokenizer_destroy(&pPathTokenizer);
  tokenizer_destroy(&pSegmentTokenizer);
}
