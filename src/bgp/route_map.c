// ==================================================================
// @(#)route_map.c
//
// @author Sebastien Tandel (sta@info.ucl.ac.be)
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 13/12/2004
// @lastdate 27/01/2005
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <string.h>

#include <libgds/array.h>
#include <libgds/hash.h>
#include <libgds/hash_utils.h>
#include <libgds/memory.h>

#include <bgp/route_map.h>

static SHash * pHashRouteMap= NULL;
static uint32_t uHashRouteMapSize= 64;

typedef struct {
  char * pcRouteMapName;
  SFilter * pFilter;
}SRouteMapHashElt;

// ----- route_map_element_compare -----------------------------------
/**
 *
 *
 */
int route_map_element_compare(void * pElt1, void * pElt2, 
			      unsigned int uEltSize)
{
  SRouteMapHashElt * pRouteMapElt1 = (SRouteMapHashElt *)pElt1;
  SRouteMapHashElt * pRouteMapElt2 = (SRouteMapHashElt *)pElt2;
  int iCompResult = strcmp(pRouteMapElt1->pcRouteMapName, 
			   pRouteMapElt2->pcRouteMapName);

  if (iCompResult < 0)
    return -1;
  else if (iCompResult > 0)
    return 1;
  else
    return 0;
}

// ------ route_map_element_destroy ----------------------------------
/**
 *
 *
 */
void route_map_element_destroy(void * pElt)
{
  SRouteMapHashElt * pRouteMapElt = (SRouteMapHashElt *)pElt;

  if (pRouteMapElt != NULL) {
    if (pRouteMapElt->pcRouteMapName != NULL)
      FREE(pRouteMapElt->pcRouteMapName);
    if (pRouteMapElt->pFilter != NULL)
      filter_destroy(&(pRouteMapElt->pFilter));
    FREE(pRouteMapElt);
    pRouteMapElt=NULL;
  } 
}

// ----- route_map_hash_compute --------------------------------------
/**
 *
 *
 */
uint32_t route_map_hash_compute(void * pElt)
{
  SRouteMapHashElt * phRouteMapElt = (SRouteMapHashElt *)pElt;

  return hash_utils_key_compute_string(phRouteMapElt->pcRouteMapName, 
					uHashRouteMapSize);
}


// ----- route_map_hash_get -----------------------------------------------
/**
 *
 *
 */
SHash * route_map_hash_get()
{
  return pHashRouteMap;
}


void _route_map_init() __attribute__((constructor));
void _route_map_finalize() __attribute__((destructor));

// ----- route_map_init ----------------------------------------------
/**
 *
 *
 */
void _route_map_init()
{
  pHashRouteMap = hash_init(uHashRouteMapSize,
			  .5,
			  route_map_element_compare,
			  route_map_element_destroy,
			  route_map_hash_compute);
}


// ----- route_map_finalize ------------------------------------------
/**
 *
 *
 */
void _route_map_finalize()
{
  SHash * phRouteMap = route_map_hash_get();
  
  hash_destroy(&phRouteMap);
}

// ----- route_map_add -----------------------------------------------
/**
 *
 *  returns the key of the hash if inserted, and 
 *  -1 if the Route Map already exists or the key is too large
 *
 */
int route_map_add(char * pcRouteMapName, SFilter * pFilter)
{
  SRouteMapHashElt * phRouteMapElt, * phRouteMapEltSearched;
  SHash * phRouteMap = route_map_hash_get();

  phRouteMapElt = MALLOC(sizeof(SRouteMapHashElt));
  phRouteMapElt->pcRouteMapName = pcRouteMapName;
  if ( (phRouteMapEltSearched = hash_search(phRouteMap, phRouteMapElt)) 
								== NULL) {
    phRouteMapEltSearched = phRouteMapElt;
    phRouteMapEltSearched->pFilter = pFilter;
    return hash_add(phRouteMap, phRouteMapEltSearched);
  } else {
    FREE(phRouteMapElt);
    LOG_DEBUG("route_map_add>Route Map %s already exists.\n",
					      pcRouteMapName);
  }

  return -1;
}

// ----- route_map_replace -------------------------------------------
/**
 *
 *
 */
int route_map_replace(char * pcRouteMapName, SFilter * pFilter)
{
  SRouteMapHashElt * phRouteMapEltSearched, * phRouteMapElt;
  SHash * phRouteMap = route_map_hash_get();

  phRouteMapElt = MALLOC(sizeof(SRouteMapHashElt));
  phRouteMapElt->pcRouteMapName = pcRouteMapName;
  if ( (phRouteMapEltSearched = hash_search(phRouteMap, phRouteMapElt)) 
							    != NULL) {
    hash_del(phRouteMap, phRouteMapEltSearched);
    phRouteMapElt->pFilter = pFilter;
    return hash_add(phRouteMap, phRouteMapElt);
  } else {
    FREE(phRouteMapElt);
    LOG_DEBUG("route_map_replace> Route Map %s wasn't existant before "
	      "the replacement.\n", pcRouteMapName);
  }

  return -1;
}

// ----- route_map_del -----------------------------------------------
/**
 *
 *
 */
int route_map_del(char * pcRouteMapName)
{
  SRouteMapHashElt * phRouteMapElt;
  SHash * phRouteMap = route_map_hash_get();

  phRouteMapElt = MALLOC(sizeof(SRouteMapHashElt));
  phRouteMapElt->pcRouteMapName = pcRouteMapName;
  if (hash_del(phRouteMap, phRouteMapElt) == 0) {
    LOG_DEBUG("route_map_del> No Route Map %s found.\n", 
				      pcRouteMapName);
  }

  FREE(phRouteMapElt);
  return 0;
}

// ----- route_map_get -----------------------------------------------
/**
 *
 *
 */
SFilter * route_map_get(char * pcRouteMapName)
{
  SRouteMapHashElt * phRouteMapEltSearched, * phRouteMapElt;
  SFilter * pFilter = NULL;
  SHash * phRouteMap = route_map_hash_get();

  phRouteMapElt = MALLOC(sizeof(SRouteMapHashElt));
  phRouteMapElt->pcRouteMapName = pcRouteMapName;
  if ( (phRouteMapEltSearched = hash_search(phRouteMap, phRouteMapElt)) 
								!= NULL) 
    pFilter = phRouteMapEltSearched->pFilter;
   else
    LOG_DEBUG("route_map_get> No Route Map %s found.\n", pcRouteMapName);

  FREE(phRouteMapElt);
  return pFilter;
}

