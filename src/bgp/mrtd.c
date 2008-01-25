// ==================================================================
// @(#)mrtd.c
//
// Interface with MRT data (ASCII and binary). The importation of
// binary MRT data is based on Dan Ardelean's library (libbgpdump).
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @author Dan Ardelean (dan@ripe.net, dardelea@cs.purdue.edu)
// @author Sebastien Tandel (standel@info.ucl.ac.be)
// 
// @date 20/02/2004
// @lastdate 20/07/2007
// ==================================================================

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
#include <net/util.h>

#ifdef HAVE_LIBZ
# include <zlib.h>
typedef gzFile FILE_TYPE;
# define FILE_OPEN(N,A) gzopen(N, A)
# define FILE_DOPEN(N,A) gzdopen(N,A)
# define FILE_CLOSE(F) gzclose(F)
# define FILE_GETS(F,B,L) gzgets(F, B, L)
# define FILE_EOF(F) gzeof(F)
#else
typedef FILE FILE_TYPE;
# define FILE_OPEN(N,A) fopen(N, A)
# define FILE_DOPEN(N,A) fdopen(N,A)
# define FILE_CLOSE(F) fclose(F)
# define FILE_GETS(F,B,L) fgets(B,L,F)
# define FILE_EOF(F) feof(F)
#endif

// ----- Local tokenizers -----
static STokenizer * pLineTokenizer= NULL;


/////////////////////////////////////////////////////////////////////
//
// ASCII MRT FUNCTIONS
//
/////////////////////////////////////////////////////////////////////

// -----[ _mrtd_get_origin ]-----------------------------------------
/**
 * This function converts the origin field of an MRT record to the
 * route origin. If the route origin is unknown, -1 is returned.
 */
static bgp_origin_t _mrtd_get_origin(const char * pcField)
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

// -----[ _mrtd_check_type ]-----------------------------------------
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
static inline int _mrtd_check_type(char * pcField, mrtd_input_t tType)
{
  return (((tType == MRTD_TYPE_RIB) &&
	   !strcmp(pcField, "TABLE_DUMP")) ||
	  ((tType != MRTD_TYPE_RIB) &&
	   (!strcmp(pcField, "BGP") || !strcmp(pcField, "BGP4")))
	  );
}

// -----[ _mrtd_check_header ]---------------------------------------
/**
 * Check an MRT route record's header. Return the MRT record type
 * which is one of 'A' (update message), 'W' (withdraw message) and
 * 'B' (best route).
 */
static inline mrtd_input_t _mrtd_check_header(STokens * pTokens)
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
  if (!_mrtd_check_type(pcTemp, tType)) {
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

// -----[ _mrtd_create_route ]---------------------------------------
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
static int _mrtd_create_route(const char * pcLine, SPrefix * pPrefix,
			      net_addr_t * ptPeerAddr,
			      unsigned int * puPeerAS,
			      SRoute ** ppRoute)
{
  mrtd_input_t tType;
  STokens * pTokens;
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

  if (pLineTokenizer == NULL)
    pLineTokenizer= tokenizer_create("|", 1, NULL, NULL);

  /* Really parse the line... */
  if (tokenizer_run(pLineTokenizer, (char *) pcLine) != TOKENIZER_SUCCESS) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: could not parse line in MRTD RIB\n");
    return -1;
  }
      
  pTokens= tokenizer_get_tokens(pLineTokenizer);

  /* Check header, length and get type (route/update/withdraw) */
  tType= _mrtd_check_header(pTokens);
  if (tType == MRTD_TYPE_INVALID) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: invalid headers\n");
    return -1;
  }

  /* Check the PeerIP field */
  pcTemp= tokens_get_string_at(pTokens, 3);
  if (str2address(pcTemp, ptPeerAddr) != 0) {
    LOG_ERR(LOG_LEVEL_SEVERE,
	    "Error: invalid PeerIP field \"%s\"\n", pcTemp);
    return -1;
  }  
  
  /* Check the PeerAS field */
  if (tokens_get_uint_at(pTokens, 4, puPeerAS) ||
      (*puPeerAS > 65535)) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: invalid PeerAS field \"%s\"\n",
	       tokens_get_string_at(pTokens, 4));
    return -1;
  }

  /* Check the prefix */
  pcTemp= tokens_get_string_at(pTokens, 5);
  if (str2prefix(pcTemp, pPrefix) != 0) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: not a valid prefix \"%s\"\n", pcTemp);
    return -1;
  }

  if (tType == MRTD_TYPE_WITHDRAW)
    return tType;

  /* Check the AS-PATH */
  pcTemp= tokens_get_string_at(pTokens, 6);
  pPath= path_from_string(pcTemp);
  if (pPath == NULL) {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: not a valid AS-Path \"%s\"\n", pcTemp);
    return -1;
  }

  /* Check ORIGIN (currently, the origin is always IGP) */
  pcTemp= tokens_get_string_at(pTokens, 7);
  tOrigin= _mrtd_get_origin(pcTemp);
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
    pComm= comm_from_string(pcTemp);
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
  route_flag_set(pRoute, ROUTE_FLAG_BEST, 1);
  route_flag_set(pRoute, ROUTE_FLAG_ELIGIBLE, 1);
  route_flag_set(pRoute, ROUTE_FLAG_FEASIBLE, 1);

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
SRoute * mrtd_route_from_line(const char * pcLine, net_addr_t * ptPeerAddr,
			      unsigned int * puPeerAS)
{
  SRoute * pRoute;
  SPrefix sPrefix;

  if (_mrtd_create_route(pcLine, &sPrefix, ptPeerAddr, puPeerAS, &pRoute)
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
			     const char * pcLine)
{
  SBGPMsg * pMsg= NULL;
  SRoute * pRoute;
  SPrefix sPrefix;
  net_addr_t tPeerAddr;
  unsigned int uPeerAS;
#if defined __EXPERIMENTAL__ && __EXPERIMENTAL_WALTON__
  net_addr_t tNextHop;
#endif

  switch (_mrtd_create_route(pcLine, &sPrefix, &tPeerAddr, &uPeerAS, &pRoute)) {
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

// -----[ mrtd_ascii_load ]------------------------------------------
/**
 * This function loads all the routes from a table dump in MRT
 * format. The filename must have previously been converted to ASCII
 * using 'route_btoa -m'.
 */
int mrtd_ascii_load(const char * pcFileName, FBGPRouteHandler fHandler,
		    void * pContext)
{
  FILE_TYPE * pFile;
  unsigned int uLineNumber= 0;
  char acFileLine[1024];
  SRoute * pRoute;
  int iError= BGP_ROUTES_INPUT_SUCCESS;
  net_addr_t tPeerAddr;
  unsigned int uPeerAS;
  int iStatus;

  // Open input file
  if ((pcFileName == NULL) || !strcmp(pcFileName, "-")) {
    pFile= FILE_DOPEN(0, "r");
  } else {
    pFile= FILE_OPEN(pcFileName, "r");
    if (pFile == NULL)
      return -1;
  }

  while ((!FILE_EOF(pFile)) && (!iError)) {
    if (FILE_GETS(pFile, acFileLine, sizeof(acFileLine)) == NULL)
      break;
    uLineNumber++;

    /* Create a route from the file line */
    pRoute= mrtd_route_from_line(acFileLine, &tPeerAddr, &uPeerAS);
    
    /* In case of error, the MRT record is ignored */
    if (pRoute == NULL) {
      iError= BGP_ROUTES_INPUT_ERROR_SYNTAX;
      break;
    }

    iStatus= BGP_ROUTES_INPUT_STATUS_OK;
    if (fHandler(iStatus, pRoute, tPeerAddr, uPeerAS, pContext) != 0) {
      iError= BGP_ROUTES_INPUT_ERROR_UNEXPECTED;
      break;
    }
    
  }
  
  FILE_CLOSE(pFile);

  return iError;
}


/////////////////////////////////////////////////////////////////////
//
// BINARY MRT FUNCTIONS
//
/////////////////////////////////////////////////////////////////////

#ifdef HAVE_BGPDUMP
# include <sys/types.h>
# include <sys/socket.h>
# include <arpa/inet.h>
# include <external/bgpdump_lib.h>
# include <external/bgpdump_formats.h>
# include <netinet/in.h>
#endif

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
  int asn_pos;
  struct assegment *assegment;
  int iIndex;
  int iResult= 0;

  /* empty AS-path */
  if (path->length == 0)
    return NULL;

  /* ASN size == 32 bits (not supported by c-bgp) */
  if (path->asn_len != ASN16_LEN) {
    LOG_ERR(LOG_LEVEL_SEVERE, "32-bits ASN are not supported.\n");
    return NULL;
  }

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
    if ((pnt + (assegment->length * path->asn_len) + AS_HEADER_SIZE) > end) {
      iResult= -1;
      break;
    }

    /* Copy each AS number into the AS-path segment */
    for (iIndex= 0; iIndex < assegment->length; iIndex++) {
      asn_pos = iIndex * path->asn_len;
      pSegment->auValue[assegment->length-iIndex-1]=
	ntohs(*(u_int16_t *) (assegment->data + asn_pos));
    }

    /* Add the segment to the AS-path */
    if (pPath == NULL)
      pPath= path_create();
    path_add_segment(pPath, pSegment);

    /* Go to next segment... */
    pnt+= (assegment->length * path->asn_len) + AS_HEADER_SIZE;
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
  SRoute * pRoute= NULL;
  SPrefix sPrefix;
  bgp_origin_t tOrigin;
  net_addr_t tNextHop;

  if (pEntry->subtype == AFI_IP) {

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

    pRoute= route_create(sPrefix, NULL, tNextHop, tOrigin);

    if ((pEntry->attr->flag & ATTR_FLAG_BIT(BGP_ATTR_AS_PATH)) != 0)
      route_set_path(pRoute, mrtd_process_aspath(pEntry->attr->aspath));

    if ((pEntry->attr->flag & ATTR_FLAG_BIT(BGP_ATTR_LOCAL_PREF)) != 0)
      route_localpref_set(pRoute, pEntry->attr->local_pref);

    if ((pEntry->attr->flag & ATTR_FLAG_BIT(BGP_ATTR_MULTI_EXIT_DISC)) != 0)
      route_med_set(pRoute, pEntry->attr->med);

    if ((pEntry->attr->flag & ATTR_FLAG_BIT(BGP_ATTR_COMMUNITIES)) != 0)
      route_set_comm(pRoute, mrtd_process_community(pEntry->attr->community));

    /*
    // DEBUG 2007/12/21
    if (ptr_array_length(pRoute->pAttr->pASPathRef) > 0) {
      int i= 0;
      unsigned int uIndex;
      for (uIndex= 0; uIndex < ptr_array_length(pRoute->pAttr->pASPathRef); uIndex++) {
	if (((SPathSegment *) pRoute->pAttr->pASPathRef->data[uIndex])->uType == 0)
	  i= 1;
      }
      if (i) {
	fprintf(stderr, "*** AS-PATH ERROR !!!\n");
	abort();
      }
    }
    */
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
SRoute * mrtd_process_entry(BGPDUMP_ENTRY * pEntry, net_addr_t * ptPeerAddr,
			    unsigned int * puPeerAS)
{
  SRoute * pRoute= NULL;

  if (pEntry->type == BGPDUMP_TYPE_MRTD_TABLE_DUMP) {
    pRoute= mrtd_process_table_dump(pEntry);
    *ptPeerAddr= ntohl(pEntry->body.mrtd_table_dump.peer_ip.v4_addr.s_addr);
    *puPeerAS= pEntry->body.mrtd_table_dump.peer_as;
  } else {
    LOG_ERR(LOG_LEVEL_SEVERE, "Error: mrtd input contains an unsupported entry.\n");
  }

  return pRoute;
}
#endif

// -----[ mrtd_binary_load ]-----------------------------------------
/**
 *
 */
#ifdef HAVE_BGPDUMP
int mrtd_binary_load(const char * pcFileName, FBGPRouteHandler fHandler,
		     void * pContext)
{
  BGPDUMP * fDump;
  BGPDUMP_ENTRY * pDumpEntry= NULL;
  SRoute * pRoute;
  int iError= BGP_ROUTES_INPUT_SUCCESS;
  int iStatus;
  net_addr_t tPeerAddr;
  unsigned int uPeerAS;

  if ((fDump= bgpdump_open_dump((char *) pcFileName)) == NULL)
    return BGP_ROUTES_INPUT_ERROR_FILE_OPEN;

  do {

    tPeerAddr= 0;
    uPeerAS= 0;
    pRoute= NULL;

    pDumpEntry= bgpdump_read_next(fDump);
    if (pDumpEntry != NULL) {
      pRoute= mrtd_process_entry(pDumpEntry, &tPeerAddr, &uPeerAS);
      bgpdump_free_mem(pDumpEntry);

      if (pRoute == NULL)
	iStatus= BGP_ROUTES_INPUT_STATUS_IGNORED;
      else
	iStatus= BGP_ROUTES_INPUT_STATUS_OK;
      
      if (fHandler(iStatus, pRoute, tPeerAddr, uPeerAS, pContext) != 0) {
	iError= BGP_ROUTES_INPUT_ERROR_UNEXPECTED;
	break;
      }
    }
      
  } while (fDump->eof == 0);
  
  bgpdump_close_dump(fDump);

  return iError;
}
#endif


/////////////////////////////////////////////////////////////////////
//
// INITIALIZATION AND FINALIZATION SECTION
//
/////////////////////////////////////////////////////////////////////

// -----[ _mrtd_destroy ]--------------------------------------------
void _mrtd_destroy()
{
  tokenizer_destroy(&pLineTokenizer);
}
