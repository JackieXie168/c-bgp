// ==================================================================
// @(#)as-level.c
//
// Provide structures and functions to handle an AS-level topology
// with business relationships. This is the foundation for handling
// AS-level topologies inferred by Subramanian et al, by CAIDA and
// by Meulle et al.
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 30/04/2007
// $Id: as-level.c,v 1.6 2008-04-11 11:03:06 bqu Exp $
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <assert.h>
#include <string.h>

#include <libgds/memory.h>
#include <libgds/stack.h>

#include <bgp/as.h>
#include <bgp/as-level.h>
#include <bgp/as-level-filter.h>
#include <bgp/as-level-types.h>
#include <bgp/as-level-util.h>
#include <bgp/caida.h>
#include <bgp/import_meulle.h>
#include <bgp/record-route.h>
#include <bgp/rexford.h>
#include <bgp/peer_t.h>
#include <net/iface.h>
#include <net/node.h>

// ----- Load AS-level topology -----
static SASLevelTopo * pTheTopo= NULL;

// ----- Tagging/filtering communities -----
/** communities used to enforce the valley-free property */
#define COMM_PROV 1
#define COMM_PEER 10

#define ASLEVEL_TOPO_FOREACH_DOMAIN(T,I,D)		\
  for (I= 0; I < ptr_array_length(T->pDomains) && (D= (SASLevelDomain*) T->pDomains->data[I]); I++)

// -----[ _aslevel_check_peer_type ]---------------------------------
static inline int _aslevel_check_peer_type(peer_type_t tPeerType)
{
  if ((tPeerType == ASLEVEL_PEER_TYPE_CUSTOMER) ||
      (tPeerType == ASLEVEL_PEER_TYPE_PEER) ||
      (tPeerType == ASLEVEL_PEER_TYPE_PROVIDER) ||
      (tPeerType == ASLEVEL_PEER_TYPE_SIBLING))
    return ASLEVEL_SUCCESS;
  return ASLEVEL_ERROR_INVALID_RELATION;
}

// -----[ _aslevel_link_create ]-------------------------------------
static inline SASLevelLink *
_aslevel_link_create(SASLevelDomain * pNeighbor, peer_type_t tPeerType)
{
  SASLevelLink * pLink= MALLOC(sizeof(SASLevelLink));
  pLink->pNeighbor= pNeighbor;
  pLink->tPeerType= tPeerType;
  return pLink;
}

// -----[ _aslevel_link_destroy ]------------------------------------
static inline void _aslevel_link_destroy(SASLevelLink ** ppLink)
{
  if (*ppLink != NULL) {
    FREE(*ppLink);
    *ppLink= NULL;
  }
}

// -----[ _aslevel_as_compare_link ] --------------------------------
static int _aslevel_as_compare_link(void * pItem1, void * pItem2,
				    unsigned int uEltSize)
{
  SASLevelLink * pLink1= *((SASLevelLink **) pItem1);
  SASLevelLink * pLink2= *((SASLevelLink **) pItem2);

  if (pLink1->pNeighbor->uASN < pLink2->pNeighbor->uASN)
    return -1;
  else if (pLink1->pNeighbor->uASN > pLink2->pNeighbor->uASN)
    return 1;
  else
    return 0;
}

// -----[ _aslevel_as_destroy_link ]---------------------------------
static void _aslevel_as_destroy_link(void * pItem)
{
  SASLevelLink * pLink= *((SASLevelLink **) pItem);

  _aslevel_link_destroy(&pLink);
}

// -----[ _aslevel_as_create ]---------------------------------------
static inline SASLevelDomain * _aslevel_as_create(uint16_t uASN)
{
  SASLevelDomain * pDomain= MALLOC(sizeof(SASLevelDomain));
  pDomain->uASN= uASN;
  pDomain->pNeighbors= ptr_array_create(ARRAY_OPTION_SORTED |
					ARRAY_OPTION_UNIQUE,
					_aslevel_as_compare_link,
					_aslevel_as_destroy_link);
  pDomain->pRouter= NULL;
  return pDomain;
}

// -----[ _aslevel_as_destroy ]--------------------------------------
static inline void _aslevel_as_destroy(SASLevelDomain ** ppDomain)
{
  if (*ppDomain != NULL) {
    if ((*ppDomain)->pNeighbors != NULL)
      ptr_array_destroy(&(*ppDomain)->pNeighbors);
    //log_printf(pLogErr, "free domain %u !", (*ppDomain)->uASN);
    FREE(*ppDomain);
    *ppDomain= NULL;
  }
}

// -----[ _aslevel_topo_compare_as ] --------------------------------
static int _aslevel_topo_compare_as(void * pItem1, void * pItem2,
				    unsigned int uEltSize)
{
  SASLevelDomain * pDomain1= *((SASLevelDomain **) pItem1);
  SASLevelDomain * pDomain2= *((SASLevelDomain **) pItem2);

  if (pDomain1->uASN < pDomain2->uASN)
    return -1;
  else if (pDomain1->uASN > pDomain2->uASN)
    return 1;
  else
    return 0;
}

// -----[ _aslevel_topo_destroy_as ]--------------------------------
static void _aslevel_topo_destroy_as(void * pItem)
{
  SASLevelDomain * pDomain= *((SASLevelDomain **) pItem);

  _aslevel_as_destroy(&pDomain);
}

// -----[ _aslevel_create_array_domains ]----------------------------
static inline SPtrArray * _aslevel_create_array_domains(int iRef)
{
  return ptr_array_create(ARRAY_OPTION_SORTED | ARRAY_OPTION_UNIQUE,
			  _aslevel_topo_compare_as,
			  (iRef?NULL:_aslevel_topo_destroy_as));
}

// -----[ aslevel_perror ]-------------------------------------------
void aslevel_perror(SLogStream * pStream, int iErrorCode)
{
  char * pcError= aslevel_strerror(iErrorCode);
  if (pcError != NULL)
    log_printf(pStream, pcError);
  else
    log_printf(pStream, "unknown error (%i)", iErrorCode);
}

// -----[ aslevel_strerror ]-----------------------------------------
char * aslevel_strerror(int iErrorCode)
{
  switch (iErrorCode) {
  case ASLEVEL_SUCCESS:
    return "success";
  case ASLEVEL_ERROR_UNEXPECTED:
    return "unexpected error";
  case ASLEVEL_ERROR_OPEN:
    return "file open error";
  case ASLEVEL_ERROR_NUM_PARAMS:
    return "invalid number of parameters";
 case ASLEVEL_ERROR_INVALID_ASNUM:
    return "invalid ASN";
  case ASLEVEL_ERROR_INVALID_RELATION:
    return "invalid business relationship";
  case ASLEVEL_ERROR_INVALID_DELAY:
    return "invalid delay";
  case ASLEVEL_ERROR_DUPLICATE_LINK:
    return "duplicate link";
  case ASLEVEL_ERROR_LOOP_LINK:
    return "loop link";
  case ASLEVEL_ERROR_NODE_EXISTS:
    return "node already exists";
  case ASLEVEL_ERROR_NO_TOPOLOGY:
    return "no topology loaded";
  case ASLEVEL_ERROR_TOPOLOGY_LOADED:
    return "topology already loaded";
  case ASLEVEL_ERROR_UNKNOWN_FORMAT:
    return "unknown topology format";
  case ASLEVEL_ERROR_CYCLE_DETECTED:
    return "topology contains cycle(s)";
  case ASLEVEL_ERROR_DISCONNECTED:
    return "topology is not connected";
  case ASLEVEL_ERROR_INCONSISTENT:
    return "topology is not consistent";
  case ASLEVEL_ERROR_UNKNOWN_FILTER:
    return "unknown topology filter";
  case ASLEVEL_ERROR_NOT_INSTALLED:
    return "topology is not installed";
  case ASLEVEL_ERROR_ALREADY_INSTALLED:
    return "topology is already installed";
  case ASLEVEL_ERROR_ALREADY_RUNNING:
    return "topology is already running";
  }
  return NULL;
}


/////////////////////////////////////////////////////////////////////
//
// TOPOLOGY MANAGEMENT
//
/////////////////////////////////////////////////////////////////////

// -----[ _aslevel_topo_cache_init ]---------------------------------
static inline void _aslevel_topo_cache_init(SASLevelTopo * pTopo)
{
  unsigned int uIndex;
  
  for (uIndex= 0; uIndex < ASLEVEL_TOPO_AS_CACHE_SIZE; uIndex++)
    pTopo->pCacheDomains[uIndex]= NULL;
}

// -----[ _aslevel_topo_cache_update_as ]----------------------------
static inline void _aslevel_topo_cache_update_as(SASLevelTopo * pTopo,
						 SASLevelDomain * pDomain)
{
  // Shift current entries
  if (ASLEVEL_TOPO_AS_CACHE_SIZE > 0)
    memmove(&pTopo->pCacheDomains[0], &pTopo->pCacheDomains[1],
	    sizeof(SASLevelDomain *)*(ASLEVEL_TOPO_AS_CACHE_SIZE-1));
  
  // Update last entry
  pTopo->pCacheDomains[ASLEVEL_TOPO_AS_CACHE_SIZE-1]= pDomain;
}

// -----[ _aslevel_topo_cache_lookup_as ]----------------------------
static inline
SASLevelDomain * _aslevel_topo_cache_lookup_as(SASLevelTopo * pTopo,
					       uint16_t uASN)
{
  unsigned int uIndex;
  
  // Start from end of cache. Stop as soon as an empty location or a
  // matching location is found.
  for (uIndex= ASLEVEL_TOPO_AS_CACHE_SIZE; uIndex > 0; uIndex--) {
    if (pTopo->pCacheDomains[uIndex-1] == NULL)
      return NULL;
    if (pTopo->pCacheDomains[uIndex-1]->uASN == uASN)
      return pTopo->pCacheDomains[uIndex-1];
  }
  return NULL;
}

// -----[ aslevel_topo_create ]------------------------------------
SASLevelTopo * aslevel_topo_create(uint8_t uAddrScheme)
{
  SASLevelTopo * pTopo= MALLOC(sizeof(SASLevelTopo));
  pTopo->pDomains= _aslevel_create_array_domains(0);
  switch (uAddrScheme) {
  case ASLEVEL_ADDR_SCH_DEFAULT:
    pTopo->fAddrMapper= aslevel_addr_sch_default_get; break;
  case ASLEVEL_ADDR_SCH_LOCAL:
    pTopo->fAddrMapper= aslevel_addr_sch_local_get; break;
  default:
    abort();
  }
  pTopo->uState= ASLEVEL_STATE_LOADED;
  _aslevel_topo_cache_init(pTopo);
  return pTopo;
}

// -----[ aslevel_topo_destroy ]-----------------------------------
void aslevel_topo_destroy(SASLevelTopo ** ppTopo)
{
  if (*ppTopo != NULL) {
    if ((*ppTopo)->pDomains != NULL)
      ptr_array_destroy(&(*ppTopo)->pDomains);
    FREE(*ppTopo);
    *ppTopo= NULL;
  }
}

// -----[ aslevel_topo_num_nodes ]---------------------------------
unsigned int aslevel_topo_num_nodes(SASLevelTopo * pTopo)
{
  return ptr_array_length(pTopo->pDomains);
}

// -----[ aslevel_topo_num_edges ]---------------------------------
unsigned int aslevel_topo_num_edges(SASLevelTopo * pTopo)
{
  unsigned int uNumEdges= 0;
  unsigned int uIndex;
  SASLevelDomain * pDomain;

  ASLEVEL_TOPO_FOREACH_DOMAIN(pTopo, uIndex, pDomain) {
    uNumEdges+= ptr_array_length(pDomain->pNeighbors);
  }
  return uNumEdges/2;
}

// -----[ aslevel_topo_add_as ]------------------------------------
SASLevelDomain * aslevel_topo_add_as(SASLevelTopo * pTopo, uint16_t uASN)
{
  SASLevelDomain * pDomain= _aslevel_as_create(uASN);
  if (ptr_array_add(pTopo->pDomains, &pDomain) < 0) {
    _aslevel_as_destroy(&pDomain);
    return NULL;
  }

  _aslevel_topo_cache_update_as(pTopo, pDomain);
  return pDomain;
}

// -----[ aslevel_topo_get_as ]--------------------------------------
SASLevelDomain * aslevel_topo_get_as(SASLevelTopo * pTopo,
				     uint16_t uASN)
{
  SASLevelDomain sDomain= { .uASN= uASN };
  SASLevelDomain * pDomain;
  unsigned int uIndex;

  // Domain in cache ?
  pDomain= _aslevel_topo_cache_lookup_as(pTopo, uASN);
  if (pDomain != NULL)
    return pDomain;

  // Full search...
  pDomain= &sDomain;
  if (ptr_array_sorted_find_index(pTopo->pDomains, &pDomain, &uIndex) == 0) {
    pDomain= (SASLevelDomain *) pTopo->pDomains->data[uIndex];
    _aslevel_topo_cache_update_as(pTopo, pDomain);
    return pDomain;
  }

  // Not found
  return NULL;
}

// -----[ aslevel_as_num_customers ]---------------------------------
unsigned int aslevel_as_num_customers(SASLevelDomain * pDomain)
{
  unsigned int uIndex;
  unsigned int uNumCustomers= 0;
  SASLevelLink * pLink;

  for (uIndex= 0; uIndex < ptr_array_length(pDomain->pNeighbors); uIndex++) {
    pLink= (SASLevelLink *) pDomain->pNeighbors->data[uIndex];
    if (pLink->tPeerType == ASLEVEL_PEER_TYPE_CUSTOMER)
      uNumCustomers++;
  }  
  
  return uNumCustomers;
}

// -----[ aslevel_as_num_providers ]---------------------------------
unsigned int aslevel_as_num_providers(SASLevelDomain * pDomain)
{
  unsigned int uIndex;
  unsigned int uNumProviders= 0;
  SASLevelLink * pLink;

  for (uIndex= 0; uIndex < ptr_array_length(pDomain->pNeighbors); uIndex++) {
    pLink= (SASLevelLink *) pDomain->pNeighbors->data[uIndex];
    if (pLink->tPeerType == ASLEVEL_PEER_TYPE_PROVIDER)
      uNumProviders++;
  }  
  
  return uNumProviders;
}

// -----[ aslevel_as_add_link ]--------------------------------------
int aslevel_as_add_link(SASLevelDomain * pDomain1,
				   SASLevelDomain * pDomain2,
				   peer_type_t tPeerType,
				   SASLevelLink ** ppLink)
{
  SASLevelLink * pLink;

  // Check peer type validity
  if (_aslevel_check_peer_type(tPeerType) != ASLEVEL_SUCCESS)
    return ASLEVEL_ERROR_INVALID_RELATION;

  // Check that link endpoints are different
  if (pDomain1->uASN == pDomain2->uASN)
    return ASLEVEL_ERROR_LOOP_LINK;

  pLink= _aslevel_link_create(pDomain2, tPeerType);

  if (ptr_array_add(pDomain1->pNeighbors, &pLink) < 0) {
    _aslevel_link_destroy(&pLink);
    return ASLEVEL_ERROR_DUPLICATE_LINK;
  }

  if (ppLink != NULL)
    *ppLink= pLink;

  return ASLEVEL_SUCCESS;;
}

// -----[ aslevel_as_get_link ]--------------------------------------
SASLevelLink * aslevel_as_get_link(SASLevelDomain * pDomain1,
				   SASLevelDomain * pDomain2)
{
  SASLevelLink sLink= { .pNeighbor= pDomain2 };
  SASLevelLink * pLink= &sLink;
  unsigned int uIndex;

  if (pDomain1->pNeighbors == NULL)
    return NULL;

  if (ptr_array_sorted_find_index(pDomain1->pNeighbors, &pLink, &uIndex) == 0)
    return (SASLevelLink *) pDomain1->pNeighbors->data[uIndex];

  return NULL;
}

// -----[ aslevel_link_get_peer_type ]-------------------------------
peer_type_t aslevel_link_get_peer_type(SASLevelLink * pLink)
{
  return pLink->tPeerType;
}

// -----[ aslevel_topo_get_top ]-------------------------------------
SPtrArray * aslevel_topo_get_top(SASLevelTopo * pTopo)
{
  unsigned int uIndex;
  SASLevelDomain * pDomain;
  SPtrArray * pArray= _aslevel_create_array_domains(1);

  ASLEVEL_TOPO_FOREACH_DOMAIN(pTopo, uIndex, pDomain) {
    if (aslevel_as_num_providers(pDomain) == 0)
      ptr_array_add(pArray, &pDomain);
  }
  return pArray;
}

typedef struct {
  SASLevelDomain * pDomain;
  unsigned int uIndex;
} SGTCtx;

// -----[ _ctx_create ]----------------------------------------------
static inline SGTCtx * _ctx_create(SASLevelDomain * pDomain)
{
  SGTCtx * pCtx= MALLOC(sizeof(SGTCtx));
  pCtx->pDomain= pDomain;
  pCtx->uIndex= 0;
  return pCtx;
}

// -----[ _ctx_destroy ]---------------------------------------------
static inline void _ctx_destroy(SGTCtx ** ppCtx)
{
  if (*ppCtx != NULL) {
    FREE(*ppCtx);
    *ppCtx= NULL;
  }
}

// -----[ aslevel_topo_check_connectedness ]-------------------------
/**
 * Start from one node and check that all other nodes are reachable
 * (regardless of the business relationships). This is a depth first
 * traversal since the depth of the topology is expected to be lower
 * than its width.
 */
int aslevel_topo_check_connectedness(SASLevelTopo * pTopo)
{
#define MAX_QUEUE_SIZE MAX_AS
  SASLevelDomain * pDomain;
  SASLevelLink * pLink;
  SStack * pStack;
  SGTCtx * pCtx;
  unsigned int uIndex;
  AS_SET_DEFINE(uASVisited);

  // No nodes => connected
  if (aslevel_topo_num_nodes(pTopo) == 0)
    return ASLEVEL_SUCCESS;
  
  // Initialize array of visited ASes
  AS_SET_INIT(uASVisited);

  pStack= stack_create(MAX_QUEUE_SIZE);

  // Start with first node
  pDomain= (SASLevelDomain *) pTopo->pDomains->data[0];
  stack_push(pStack, _ctx_create(pDomain));
  AS_SET_PUT(uASVisited, pDomain->uASN);

  // Depth-first traversal of the graph (regardless of policies).
  // Mark each AS as it is reached.
  while (!stack_is_empty(pStack)) {
    pCtx= (SGTCtx *) stack_top(pStack);

    if (pCtx->uIndex < ptr_array_length(pCtx->pDomain->pNeighbors)) {

      pLink= (SASLevelLink *) pCtx->pDomain->pNeighbors->data[pCtx->uIndex];
      if (!IS_IN_AS_SET(uASVisited, pLink->pNeighbor->uASN)) {
	AS_SET_PUT(uASVisited, pLink->pNeighbor->uASN);
	assert(stack_push(pStack, _ctx_create(pLink->pNeighbor)) == 0);
      }
      pCtx->uIndex++;

    } else {
      pCtx= stack_pop(pStack);
      _ctx_destroy(&pCtx);
    }

  }

  stack_destroy(&pStack);

  // Check that every domain was reached
  for (uIndex= 0; uIndex < aslevel_topo_num_nodes(pTopo); uIndex++) {
    pDomain= (SASLevelDomain *) pTopo->pDomains->data[uIndex];
    if (!IS_IN_AS_SET(uASVisited, pDomain->uASN))
      return ASLEVEL_ERROR_DISCONNECTED;
  }

  return ASLEVEL_SUCCESS;
#undef MAX_QUEUE_SIZE
}

// -----[ aslevel_topo_check_cycle ]---------------------------------
/**
 * Start from top nodes (those that have no provider) and go down the
 * hierarchy strictly following provider->customer edges. Check that
 * no cycle is found.
 */
int aslevel_topo_check_cycle(SASLevelTopo * pTopo, int iVerbose)
{
#define MAX_QUEUE_SIZE 100
  SPtrArray * pTopDomains;
  SASLevelDomain * pDomain, * pCycleDomain;
  SASLevelLink * pLink;
  SStack * pStack;
  SGTCtx * pCtx;
  unsigned int uIndx, uIndex, uIndex2;
  int iCycle;
  int iNumCycles= 0;
  AS_SET_DEFINE(uASInCycle);
  unsigned int uNumPaths;
  unsigned int uMaxPathLength;
  unsigned int uNumReached;
  AS_SET_DEFINE(uASReached);
  AS_SET_DEFINE(uASVisited);

  pTopDomains= aslevel_topo_get_top(pTopo);

  // No nodes => no cycle
  if (aslevel_topo_num_nodes(pTopo) == 0)
    return ASLEVEL_SUCCESS;

  // No top domain => topology is composed of a unique cycle
  if (ptr_array_length(pTopDomains) == 0)
    return ASLEVEL_ERROR_CYCLE_DETECTED;

  // Initialize array of visited ASes
  AS_SET_INIT(uASInCycle);

  //fprintf(stderr, "# top domains: %d\n", ptr_array_length(pTopDomains));

  // Starting from each top AS, traverse all the p2c edges
  iCycle= 0;
  for (uIndx= 0; (uIndx < ptr_array_length(pTopDomains)) && !iCycle; uIndx++) {
    pStack= stack_create(MAX_QUEUE_SIZE);

    pDomain= (SASLevelDomain *) pTopDomains->data[uIndx];
    assert(stack_push(pStack, _ctx_create(pDomain)) == 0);

    if (iVerbose)
      fprintf(stderr, "-> from AS%d", pDomain->uASN);

    AS_SET_INIT(uASReached);
    AS_SET_PUT(uASReached, pDomain->uASN);
    uNumPaths= 0;
    uMaxPathLength= 0;

    AS_SET_INIT(uASVisited);
    AS_SET_PUT(uASVisited, pDomain->uASN);

    iCycle= 0;
    while (!stack_is_empty(pStack) && !iCycle) {
      pCtx= (SGTCtx *) stack_top(pStack);

      AS_SET_PUT(uASReached, pCtx->pDomain->uASN);
      if (pCtx->pDomain == pDomain) {
	//fprintf(stderr, "\r-> from AS%d            ", pDomain->uASN);
	if (pCtx->uIndex < ptr_array_length(pCtx->pDomain->pNeighbors)) {
	  pLink= (SASLevelLink *) pCtx->pDomain->pNeighbors->data[pCtx->uIndex];
	  if (iVerbose)
	    fprintf(stderr, "\r-> from AS%d (%d:AS%d)",
		    pDomain->uASN, pCtx->uIndex, pLink->pNeighbor->uASN);
	} else
	  if (iVerbose)
	    fprintf(stderr, "\r-> from AS%d (completed)", pDomain->uASN);
      }

      if (pCtx->uIndex < ptr_array_length(pCtx->pDomain->pNeighbors)) {
	
	pLink= (SASLevelLink *) pCtx->pDomain->pNeighbors->data[pCtx->uIndex];
	if (!IS_IN_AS_SET(uASInCycle, pLink->pNeighbor->uASN) &&
	    (pLink->tPeerType == ASLEVEL_PEER_TYPE_CUSTOMER)) {
	  iCycle= 0;

	  if (IS_IN_AS_SET(uASVisited, pLink->pNeighbor->uASN)) {

	    for (uIndex= 0; uIndex < stack_depth(pStack); uIndex++)
	      if (((SGTCtx *) stack_get_at(pStack, uIndex))->pDomain == pLink->pNeighbor) {
		if (iVerbose)
		  log_printf(pLogErr, "\rcycle detected:");
		// Mark all ASes in the cycle
		for (uIndex2= uIndex; uIndex2 < stack_depth(pStack); uIndex2++) {
		  pCycleDomain= ((SGTCtx *) stack_get_at(pStack, uIndex2))->pDomain;
		  if (iVerbose)
		    log_printf(pLogErr, " %u", pCycleDomain->uASN);
		  AS_SET_PUT(uASInCycle, pCycleDomain->uASN);
		}
		if (iVerbose)
		  log_printf(pLogErr, " %u\n", pLink->pNeighbor->uASN);
		
		iCycle= 1;
		iNumCycles++;
		break;
	      }
	  }

	  if (iCycle == 0) {
	    assert(stack_push(pStack, _ctx_create(pLink->pNeighbor)) == 0);
	    AS_SET_PUT(uASVisited, pLink->pNeighbor->uASN);
	    if (stack_depth(pStack) > uMaxPathLength)
	      uMaxPathLength= stack_depth(pStack);
	  }
	  iCycle= 0;
	}

	pCtx->uIndex++;
	
      } else {
	pCtx= stack_pop(pStack);
	AS_SET_CLEAR(uASVisited, pCtx->pDomain->uASN);
	_ctx_destroy(&pCtx);
      }

    }

    if (iVerbose) {
      fprintf(stderr, "\n");

      uNumReached= 0;
      for (uIndex2= 0; uIndex2 < MAX_AS; uIndex2++) {
	if (IS_IN_AS_SET(uASReached, uIndex2))
	  uNumReached++;
      }
      fprintf(stderr, "\tnumber of paths traversed : %d\n", uNumPaths);
      fprintf(stderr, "\tmax path length           : %d\n", uMaxPathLength);
      fprintf(stderr, "\tnumber of domains reached : %d\n", uNumReached);
      fprintf(stderr, "\tnumber of direct customers: %d\n",
	      aslevel_as_num_customers(pDomain));
    }
      
  }

  // Clear stack (if not empty)
  while (!stack_is_empty(pStack)) {
    pCtx= stack_top(pStack);
    _ctx_destroy(&pCtx);
  }
  stack_destroy(&pStack);

  ptr_array_destroy(&pTopDomains);

  if (iNumCycles > 0)
    return ASLEVEL_ERROR_CYCLE_DETECTED;

  return ASLEVEL_SUCCESS;
}

// -----[ aslevel_topo_check_consistency ]---------------------------
/**
 * Check that all links are defined in both directions. Check that
 * both directions are tagged with matching business relationships.
 */
int aslevel_topo_check_consistency(SASLevelTopo * pTopo)
{
  unsigned int uIndex, uIndex2;
  SASLevelDomain * pDomain;
  SASLevelLink * pLink1, * pLink2;

  for (uIndex= 0; uIndex < ptr_array_length(pTopo->pDomains); uIndex++) {
    pDomain= (SASLevelDomain *) pTopo->pDomains->data[uIndex];

    for (uIndex2= 0; uIndex2 < ptr_array_length(pDomain->pNeighbors);
	 uIndex2++) {
      pLink1= (SASLevelLink *) pDomain->pNeighbors->data[uIndex2];

      // Check existence of reverse direction
      pLink2= aslevel_as_get_link(pLink1->pNeighbor, pDomain);
      if (pLink2 == NULL)
	return ASLEVEL_ERROR_INCONSISTENT;

      // Check coherence of business relationships
      if (pLink1->tPeerType != aslevel_reverse_relation(pLink2->tPeerType))
	return ASLEVEL_ERROR_INCONSISTENT;
    }
  }

  return ASLEVEL_SUCCESS;
}

// -----[ _aslevel_build_bgp_router ]--------------------------------
static inline bgp_router_t * _aslevel_build_bgp_router(net_addr_t addr,
						       uint16_t uASN)
{
  net_node_t * node;
  bgp_router_t * router= NULL;
  net_protocol_t * protocol;
  net_error_t error;

  node= network_find_node(network_get_default(), addr);
  if (node == NULL) {
    error= node_create(addr, &node);
    if (error != ESUCCESS)
      return NULL;
    error= bgp_add_router(uASN, node, &router);
    if (error != ESUCCESS)
      return NULL;
    error= network_add_node(network_get_default(), node);
    if (error != ESUCCESS)
      return NULL;
  } else {
    protocol= protocols_get(node->protocols, NET_PROTOCOL_BGP);
    if (protocol != NULL)
      router= (bgp_router_t *) protocol->pHandler;
  }
  return router;
}

// -----[ _aslevel_build_bgp_session ]-------------------------------
static inline int _aslevel_build_bgp_session(bgp_router_t * router1,
					     bgp_router_t * router2)
{
  net_node_t * node1= router1->pNode;
  net_node_t * node2= router2->pNode;
  ip_pfx_t prefix;
  net_iface_t * iface;
  igp_weight_t weight= 0;
  int result;

  // Add the link and check if it did not already exist
  result= net_link_create_rtr(node1, node2, BIDIR, &iface);
  if (result != ESUCCESS)
    return result;

  // Configure IGP weight in forward direction
  assert(iface != NULL);
  assert(net_iface_set_metric(iface, 0, weight, BIDIR) == ESUCCESS);
      
  // Add static route in forward direction
  prefix.tNetwork= node2->addr;
  prefix.uMaskLen= 32;
  assert(!node_rt_add_route(node1, prefix, net_iface_id_rtr(node2),
			    node2->addr, weight, NET_ROUTE_STATIC));

  // Add static route in reverse direction
  prefix.tNetwork= node1->addr;
  prefix.uMaskLen= 32;
  assert(!node_rt_add_route(node2, prefix, net_iface_id_rtr(node1),
			    node1->addr, weight, NET_ROUTE_STATIC));
  
  // Setup peering relations in both directions
  if (bgp_router_add_peer(router1, router2->uASN, node2->addr, NULL) != 0)
    return -1;
  if (bgp_router_add_peer(router2, router1->uASN, node1->addr, NULL) != 0)
    return -1;

  return 0;
}

// -----[ aslevel_topo_build_network ]-------------------------------
int aslevel_topo_build_network(SASLevelTopo * pTopo)
{
  unsigned int uIndex, uIndex2;
  SASLevelDomain * pDomain, * pNeighbor;
  net_addr_t addr;
  bgp_router_t * pRouter;

  if (pTopo->uState != ASLEVEL_STATE_LOADED)
    return ASLEVEL_ERROR_ALREADY_INSTALLED;

  // ****************************************************
  // *   Check that none of the routers already exist   *
  // ****************************************************
  for (uIndex= 0; uIndex < ptr_array_length(pTopo->pDomains); uIndex++) {
    pDomain= (SASLevelDomain *) pTopo->pDomains->data[uIndex];

    // Determine node addresses (identifiers)
    addr= pTopo->fAddrMapper(pDomain->uASN);

    // Check that node doesn't exist
    if (network_find_node(network_get_default(), addr) != NULL)
      return ASLEVEL_ERROR_NODE_EXISTS;
  }

  // ********************************
  // *   Create all nodes/routers   *
  // ********************************
  for (uIndex= 0; uIndex < ptr_array_length(pTopo->pDomains); uIndex++) {
    pDomain= (SASLevelDomain *) pTopo->pDomains->data[uIndex];

    // Determine node addresses (identifiers)
    addr= pTopo->fAddrMapper(pDomain->uASN);

    // Find/create AS1's node
    pRouter= _aslevel_build_bgp_router(addr, pDomain->uASN);
    if (pRouter == NULL)
      return ASLEVEL_ERROR_UNEXPECTED;

    pDomain->pRouter= pRouter;
  }

  // *********************************
  // *   Create all links/sessions   *
  // *********************************
  for (uIndex= 0; uIndex < ptr_array_length(pTopo->pDomains); uIndex++) {
    pDomain= (SASLevelDomain *) pTopo->pDomains->data[uIndex];

    for (uIndex2= 0; uIndex2 < ptr_array_length(pDomain->pNeighbors);
	 uIndex2++) {
      pNeighbor= ((SASLevelLink *) pDomain->pNeighbors->data[uIndex2])->pNeighbor;

      if (pDomain->uASN < pNeighbor->uASN)
	continue;

      if (_aslevel_build_bgp_session(pDomain->pRouter,
				     pNeighbor->pRouter) != 0)
	return ASLEVEL_ERROR_UNEXPECTED;
    }
  }

  pTopo->uState= ASLEVEL_STATE_INSTALLED;

  return ASLEVEL_SUCCESS;
}

// -----[ aslevel_topo_setup_policies ]------------------------------
int aslevel_topo_setup_policies(SASLevelTopo * pTopo)
{
  unsigned int uIndex, uIndex2;
  SASLevelDomain * pDomain, * pNeighbor;
  SASLevelLink * pLink;
  bgp_filter_t * pFilter;

  if (pTopo->uState < ASLEVEL_STATE_INSTALLED)
    return ASLEVEL_ERROR_NOT_INSTALLED;
  if (pTopo->uState > ASLEVEL_STATE_POLICIES)
    return ASLEVEL_ERROR_ALREADY_RUNNING;

  for (uIndex= 0; uIndex < ptr_array_length(pTopo->pDomains); uIndex++) {
    pDomain= (SASLevelDomain *) pTopo->pDomains->data[uIndex];

    for (uIndex2= 0; uIndex2 < ptr_array_length(pDomain->pNeighbors);
	 uIndex2++) {
      pLink= (SASLevelLink *) pDomain->pNeighbors->data[uIndex2];
      pNeighbor= pLink->pNeighbor;

      pFilter= aslevel_filter_in(pLink->tPeerType);
      bgp_router_peer_set_filter(pDomain->pRouter,
				 pNeighbor->pRouter->pNode->addr,
				 FILTER_IN, pFilter);
      pFilter= aslevel_filter_out(pLink->tPeerType);
      bgp_router_peer_set_filter(pDomain->pRouter,
				 pNeighbor->pRouter->pNode->addr,
				 FILTER_OUT, pFilter);

    }
  }

  pTopo->uState= ASLEVEL_STATE_POLICIES;

  return ASLEVEL_SUCCESS;
}

// -----[ aslevel_topo_load ]----------------------------------------
int aslevel_topo_load(const char * pcFileName, uint8_t uFormat,
		      uint8_t uAddrScheme)
{
  FILE * pFile;
  int iResult;
  unsigned int uLineNumber;

  if (pTheTopo != NULL)
    return ASLEVEL_ERROR_TOPOLOGY_LOADED;

  if ((pFile= fopen(pcFileName, "r")) != NULL) {

    pTheTopo= aslevel_topo_create(uAddrScheme);

    // Parse input file
    switch (uFormat) {
    case ASLEVEL_FORMAT_REXFORD:
      iResult= rexford_parser(pFile, pTheTopo, &uLineNumber);
      break;
    case ASLEVEL_FORMAT_CAIDA:
      iResult= caida_parser(pFile, pTheTopo, &uLineNumber);
      break;
    case ASLEVEL_FORMAT_MEULLE:
      iResult= meulle_parser(pFile, pTheTopo, &uLineNumber);
      break;
    default:
      iResult= ASLEVEL_ERROR_UNKNOWN_FORMAT;
    }

    if (iResult != 0) {
      LOG_ERR(LOG_LEVEL_SEVERE, "Error: while parsing file \"%s\", line %u\n",
	      pcFileName, uLineNumber);
      aslevel_topo_destroy(&pTheTopo);
      return iResult;
    }

    fclose(pFile);
  } else
    return ASLEVEL_ERROR_OPEN;

  return aslevel_topo_check_consistency(pTheTopo);
}

// -----[ aslevel_topo_install ]-------------------------------------
int aslevel_topo_install()
{
  if (pTheTopo == NULL)
    return ASLEVEL_ERROR_NO_TOPOLOGY;

  return aslevel_topo_build_network(pTheTopo);
}

// -----[ aslevel_topo_check ]---------------------------------------
int aslevel_topo_check(iVerbose)
{
  int iResult;

  if (pTheTopo == NULL)
    return ASLEVEL_ERROR_NO_TOPOLOGY;

  if ((iResult= aslevel_topo_check_connectedness(pTheTopo))
      != ASLEVEL_SUCCESS)
    return iResult;
  
  if ((iResult= aslevel_topo_check_cycle(pTheTopo, iVerbose))
      != ASLEVEL_SUCCESS)
    return iResult;

  return ASLEVEL_SUCCESS;
}

// -----[ aslevel_topo_run ]-----------------------------------------
int aslevel_topo_run()
{
  unsigned int uIndex;
  SASLevelDomain * pDomain;

  if (pTheTopo == NULL)
    return ASLEVEL_ERROR_NO_TOPOLOGY;

  if (pTheTopo->uState < ASLEVEL_STATE_INSTALLED)
    return ASLEVEL_ERROR_NOT_INSTALLED;

  for (uIndex= 0; uIndex < ptr_array_length(pTheTopo->pDomains); uIndex++) {
    pDomain= (SASLevelDomain *) pTheTopo->pDomains->data[uIndex];
    if (bgp_router_start(pDomain->pRouter) != 0)
      return ASLEVEL_ERROR_UNEXPECTED;
  }

  pTheTopo->uState= ASLEVEL_STATE_RUNNING;

  return ASLEVEL_SUCCESS;
}

// -----[ aslevel_topo_policies ]------------------------------------
int aslevel_topo_policies()
{
  if (pTheTopo == NULL)
    return ASLEVEL_ERROR_NO_TOPOLOGY;

  return aslevel_topo_setup_policies(pTheTopo);
}

// -----[ aslevel_topo_info ]----------------------------------------
int aslevel_topo_info(SLogStream * pStream)
{
  unsigned int uNumEdges= 0;
  unsigned int uNumP2P= 0, uNumP2C= 0, uNumS2S= 0, uNumUnknown= 0;
  unsigned int uIndex, uIndex2;
  SASLevelDomain * pDomain;

  if (pTheTopo == NULL)
    return ASLEVEL_ERROR_NO_TOPOLOGY;

  log_printf(pStream, "Number of nodes: %u\n",
	     ptr_array_length(pTheTopo->pDomains));
  ASLEVEL_TOPO_FOREACH_DOMAIN(pTheTopo, uIndex, pDomain) {
    for (uIndex2= 0; uIndex2 < ptr_array_length(pDomain->pNeighbors); uIndex2++) {
      uNumEdges++;
      switch (((SASLevelLink *) pDomain->pNeighbors->data[uIndex2])->tPeerType) {
      case ASLEVEL_PEER_TYPE_CUSTOMER:
      case ASLEVEL_PEER_TYPE_PROVIDER:
	uNumP2C++;
	break;
      case ASLEVEL_PEER_TYPE_PEER:
	uNumP2P++;
	break;
      case ASLEVEL_PEER_TYPE_SIBLING:
	uNumS2S++;
	break;
      default:
	uNumUnknown++;
      }
    }
  }
  uNumEdges/= 2;
  uNumP2C/= 2;
  uNumP2P/= 2;
  uNumS2S/= 2;

  log_printf(pStream, "Number of edges: %u\n", uNumEdges);
  log_printf(pStream, "  (p2c:%d / p2p:%d / s2s: %d)\n",
	     uNumP2C, uNumP2P, uNumS2S);
  log_printf(pStream, "Number of unknown relationships: %u\n", uNumUnknown);

  log_printf(pStream, "State          : %u\n", pTheTopo->uState);

  return ASLEVEL_SUCCESS;
}

// -----[ aslevel_topo_filter ]--------------------------------------
/**
 * This function filters some domains/links from the AS-level
 * topology. Current filters are supported:
 * - stubs             
 * - single-homed-stubs
 *
 * The topology must be in state LOADED
 */
int aslevel_topo_filter(uint8_t uFilter)
{
  if (pTheTopo == NULL)
    return ASLEVEL_ERROR_NO_TOPOLOGY;

  if (pTheTopo->uState > ASLEVEL_STATE_LOADED)
    return ASLEVEL_ERROR_ALREADY_INSTALLED;

  return aslevel_filter_topo(pTheTopo, uFilter);
}

// -----[ aslevel_topo_dump_stubs ]----------------------------------
int aslevel_topo_dump_stubs(SLogStream * pStream)
{
  unsigned int uIndex, uIndex2;
  unsigned int uNumCustomers= 0;
  SASLevelDomain * pDomain;
  SASLevelLink * pLink;

  if (pTheTopo == NULL)
    return ASLEVEL_ERROR_NO_TOPOLOGY;

  // Identify domains to be removed
  for (uIndex= 0; uIndex < aslevel_topo_num_nodes(pTheTopo); uIndex++) {
    pDomain= (SASLevelDomain *) pTheTopo->pDomains->data[uIndex];
    uNumCustomers= aslevel_as_num_customers(pDomain);
    if (uNumCustomers == 0) {
      log_printf(pStream, "%u", pDomain->uASN);
      for (uIndex2= 0; uIndex2 < ptr_array_length(pDomain->pNeighbors);
	   uIndex2++) {
	pLink= (SASLevelLink *) pDomain->pNeighbors->data[uIndex2];
	log_printf(pStream, " %u", pLink->pNeighbor->uASN);
      }
      log_printf(pStream, "\n");
    }
  }

  return ASLEVEL_SUCCESS;
}

// -----[ aslevel_get_topo ]-----------------------------------------
SASLevelTopo * aslevel_get_topo()
{
  return pTheTopo;
}


// -----[ aslevel_topo_record_route ]--------------------------------
int aslevel_topo_record_route(SLogStream * pStream,
			      SPrefix sPrefix)
{
  unsigned int uIndex;
  int iResult;
  SBGPPath * pPath= NULL;
  SASLevelDomain * pDomain;

  if (pTheTopo == NULL)
    return ASLEVEL_ERROR_NO_TOPOLOGY;

  if (pTheTopo->uState < ASLEVEL_STATE_INSTALLED)
    return ASLEVEL_ERROR_NOT_INSTALLED;

  for (uIndex= 0; uIndex < ptr_array_length(pTheTopo->pDomains); uIndex++) {
    pDomain= (SASLevelDomain *) pTheTopo->pDomains->data[uIndex];
    iResult= bgp_record_route(pDomain->pRouter, sPrefix, &pPath, 0);
    bgp_dump_recorded_route(pStream, pDomain->pRouter, sPrefix,
			    pPath, iResult);
    path_destroy(&pPath);
  }

  return 0;
}

// -----[ aslevel_topo_str2format ]----------------------------------
int aslevel_topo_str2format(const char * pcFormat, uint8_t * puFormat)
{
  if (!strcmp(pcFormat, "default") || !strcmp(pcFormat, "rexford")) {
    *puFormat= ASLEVEL_FORMAT_REXFORD;
    return ASLEVEL_SUCCESS;
  } else if (!strcmp(pcFormat, "caida")) {
    *puFormat= ASLEVEL_FORMAT_CAIDA;
    return ASLEVEL_SUCCESS;
  } else if (!strcmp(pcFormat, "meulle")) {
    *puFormat= ASLEVEL_FORMAT_MEULLE;
    return ASLEVEL_SUCCESS;
  }
  return ASLEVEL_ERROR_UNKNOWN_FORMAT;
}


/////////////////////////////////////////////////////////////////////
//
// FILTERS MANAGEMENT
//
/////////////////////////////////////////////////////////////////////

// -----[ aslevel_str2peer_type ]------------------------------------
peer_type_t aslevel_str2peer_type(const char * pcStr)
{
  if (!strcmp(pcStr, "customer")) {
    return ASLEVEL_PEER_TYPE_CUSTOMER;
  } else if (!strcmp(pcStr, "peer")) {
    return ASLEVEL_PEER_TYPE_PEER;
  } else if (!strcmp(pcStr, "provider")) {
    return ASLEVEL_PEER_TYPE_PROVIDER;
  } else if (!strcmp(pcStr, "sibling")) {
    return ASLEVEL_PEER_TYPE_SIBLING;
  } 
  return ASLEVEL_PEER_TYPE_UNKNOWN;
}

// -----[ aslevel_reverse_relation ]---------------------------------
peer_type_t aslevel_reverse_relation(peer_type_t tPeerType)
{
  switch (tPeerType) {
  case ASLEVEL_PEER_TYPE_CUSTOMER:
    return ASLEVEL_PEER_TYPE_PROVIDER;
  case ASLEVEL_PEER_TYPE_PROVIDER:
    return ASLEVEL_PEER_TYPE_CUSTOMER;
  case ASLEVEL_PEER_TYPE_PEER:
    return ASLEVEL_PEER_TYPE_PEER;
  case ASLEVEL_PEER_TYPE_SIBLING:
    return ASLEVEL_PEER_TYPE_SIBLING;
  }
  return ASLEVEL_PEER_TYPE_UNKNOWN;
}

// -----[ aslevel_filter_in ]----------------------------------------
/**
 * Create an input filter based on the peer type.
 */
bgp_filter_t * aslevel_filter_in(peer_type_t tPeerType)
{
  bgp_filter_t * pFilter= NULL;

  switch (tPeerType) {
  case ASLEVEL_PEER_TYPE_CUSTOMER:
    pFilter= filter_create();
    filter_add_rule(pFilter, NULL, filter_action_pref_set(ASLEVEL_PREF_CUST));
    break;
  case ASLEVEL_PEER_TYPE_PROVIDER:
    pFilter= filter_create();
    filter_add_rule(pFilter, NULL, filter_action_comm_append(COMM_PROV));
    filter_add_rule(pFilter, NULL, filter_action_pref_set(ASLEVEL_PREF_PROV));
    break;
  case ASLEVEL_PEER_TYPE_PEER:
    pFilter= filter_create();
    filter_add_rule(pFilter, NULL, filter_action_comm_append(COMM_PEER));
    filter_add_rule(pFilter, NULL, filter_action_pref_set(ASLEVEL_PREF_PEER));
    break;
  case ASLEVEL_PEER_TYPE_SIBLING:
    break;
  }
  return pFilter;
}

// -----[ aslevel_filter_out ]---------------------------------------
/**
 * Create an output filter based on the peer type.
 */
bgp_filter_t * aslevel_filter_out(peer_type_t tPeerType)
{
  bgp_filter_t * pFilter= NULL;

  switch (tPeerType) {
  case ASLEVEL_PEER_TYPE_CUSTOMER:
    pFilter= filter_create();
    filter_add_rule(pFilter, NULL, filter_action_comm_remove(COMM_PROV));
    filter_add_rule(pFilter, NULL, filter_action_comm_remove(COMM_PEER));
    break;
  case ASLEVEL_PEER_TYPE_PROVIDER:
    pFilter= filter_create();
    filter_add_rule(pFilter, FTM_OR(filter_match_comm_contains(COMM_PROV),
				    filter_match_comm_contains(COMM_PEER)),
		    FTA_DENY);
    filter_add_rule(pFilter, NULL, filter_action_comm_remove(COMM_PROV));
    filter_add_rule(pFilter, NULL, filter_action_comm_remove(COMM_PEER));
    break;
  case ASLEVEL_PEER_TYPE_PEER:
    pFilter= filter_create();
    filter_add_rule(pFilter,
		    FTM_OR(filter_match_comm_contains(COMM_PROV),
			   filter_match_comm_contains(COMM_PEER)),
		    FTA_DENY);
    filter_add_rule(pFilter, NULL, filter_action_comm_remove(COMM_PROV));
    filter_add_rule(pFilter, NULL, filter_action_comm_remove(COMM_PEER));
    break;
  case ASLEVEL_PEER_TYPE_SIBLING:
    break;
  }
  return pFilter;
}


/////////////////////////////////////////////////////////////////////
//
// ADDRESSING SCHEMES MANAGEMENT
//
/////////////////////////////////////////////////////////////////////

// -----[ aslevel_str2addr_sch ]-------------------------------------
int aslevel_str2addr_sch(const char * pcStr, uint8_t * puAddrScheme)
{
  if (!strcmp(pcStr, "global") || !strcmp(pcStr, "default")) {
    *puAddrScheme= ASLEVEL_ADDR_SCH_GLOBAL;
    return ASLEVEL_SUCCESS;
  } else if (!strcmp(pcStr, "local")) {
    *puAddrScheme= ASLEVEL_ADDR_SCH_LOCAL;
    return ASLEVEL_SUCCESS;
  }
  return ASLEVEL_ERROR_UNKNOWN_ADDRSCH;
}

// -----[ aslevel_addr_sch_default_get ]----------------------------
/**
 * Return an IP address in the local prefix 0.0.0.0/8. The address is
 * derived from the AS number as follows:
 *
 *   addr = ASNH.ASNL.0.0
 *
 * where ASNH is composed of the 8 most significant bits of the ASN
 * while ASNL is composed of the 8 less significant bits of the ASN.
 */
net_addr_t aslevel_addr_sch_default_get(uint16_t uASN)
{
  return (uASN << 16);
}

// -----[ aslevel_addr_sch_local_get ]-------------------------------
/**
 * Return an IP address in the local prefix 0.0.0.0/8. The address is
 * derived from the AS number as follows:
 *
 *   addr = 0.0.ASNH.ASNL
 *
 * where ASNH is composed of the 8 most significant bits of the ASN
 * while ASNL is composed of the 8 less significant bits of the ASN.
 */
net_addr_t aslevel_addr_sch_local_get(uint16_t uASN)
{
  return uASN;
}


/////////////////////////////////////////////////////////////////////
//
// INITIALIZATION & FINALIZATION
//
/////////////////////////////////////////////////////////////////////

// -----[ _aslevel_destroy ]-----------------------------------------
void _aslevel_destroy()
{
  if (pTheTopo != NULL)
    aslevel_topo_destroy(&pTheTopo);
}
