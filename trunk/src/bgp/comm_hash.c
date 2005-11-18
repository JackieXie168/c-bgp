// ==================================================================
// @(#)comm_hash.c
//
// This is the global Communities repository.
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 14/10/2005
// @lastdate 14/10/2005
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <libgds/array.h>
#include <libgds/hash.h>
#include <libgds/hash_utils.h>

#include <bgp/comm.h>
#include <bgp/comm_hash.h>

// ---| Function prototypes |---
static uint32_t _comm_hash_item_compute(void * pItem, uint32_t uHashSize);

// ---| Private parameters |---
static SHash * pCommHash= NULL;
static uint32_t uCommHashSize= 25000;
static uint8_t uCommHashMethod= COMM_HASH_METHOD_STRING;
static FHashCompute fCommHashCompute= _comm_hash_item_compute;

// -----[ _comm_hash_item_compute ]----------------------------------
/**
 * This is a helper function that computes the hash key of an
 * Communities attribute. The function relies on a static buffer in
 * order to avoid frequent memory allocation.
 */
#define AS_COMM_STR_SIZE 1024
static char acCommStr1[AS_COMM_STR_SIZE];
static char acCommStr2[AS_COMM_STR_SIZE];
static uint32_t _comm_hash_item_compute(void * pItem, uint32_t uHashSize)
{
  assert(comm_to_string((SCommunities *) pItem, acCommStr1,
			AS_COMM_STR_SIZE) < AS_COMM_STR_SIZE);
  return hash_utils_key_compute_string(acCommStr1, uCommHashSize)%uHashSize;
}

// -----[ _comm_hash_item_compute_zebra ]----------------------------
/**
 * This is a helper function that computes the hash key of an
 * Communities attribute. The function is based on Zebra's Communities
 * attribute hashing function.
 */
static uint32_t _comm_hash_item_compute_zebra(void * pItem, uint32_t uHashSize)
{
  SCommunities * pCommunities= (SCommunities *) pItem;
  unsigned int uKey= 0;
  int iIndex;
  comm_t tCommunity;

  for (iIndex= 0; iIndex < pCommunities->iSize; iIndex++) {
    tCommunity= (comm_t) pCommunities->ppItems[iIndex];
    uKey+= tCommunity & 255;
    tCommunity= tCommunity >> 8;
    uKey+= tCommunity & 255;
    tCommunity= tCommunity >> 8;
    uKey+= tCommunity & 255;
    tCommunity= tCommunity >> 8;
    uKey+= tCommunity;
  }
  return uKey%uHashSize;
}

// -----[ comm_hash_item_compare ]-----------------------------------
int _comm_hash_item_compare(void * pComm1, void * pComm2,
			    uint32_t uEltSize)
{
  assert(comm_to_string(pComm1, acCommStr1, AS_COMM_STR_SIZE)
	 < AS_COMM_STR_SIZE);
  assert(comm_to_string(pComm2, acCommStr2, AS_COMM_STR_SIZE)
	 < AS_COMM_STR_SIZE);

  return strcmp(acCommStr1, acCommStr2);
}

// -----[ comm_hash_item_destroy ]-----------------------------------
void _comm_hash_item_destroy(void * pItem)
{
  SCommunities * pCommunities= (SCommunities *) pItem;
  comm_destroy(&pCommunities);
}

// -----[ comm_hash_add ]--------------------------------------------
/**
 *
 */
int comm_hash_add(SCommunities * pCommunities)
{
  _comm_hash_init();
  return hash_add(pCommHash, pCommunities);
}

// -----[ comm_hash_add ]--------------------------------------------
/**
 *
 */
SCommunities * comm_hash_get(SCommunities * pCommunities)
{
  _comm_hash_init();
  return (SCommunities *) hash_search(pCommHash, pCommunities);
}

// -----[ comm_hash_add ]--------------------------------------------
/**
 *
 */
int comm_hash_remove(SCommunities * pCommunities)
{
  _comm_hash_init();
  return hash_del(pCommHash, pCommunities);
}

// -----[ comm_hash_get_size ]---------------------------------------
/**
 * Return the Communities hash size.
 */
uint32_t comm_hash_get_size()
{
  return uCommHashSize;
}

// -----[ comm_hash_set_size ]---------------------------------------
/**
 * Change the Communities hash size. This is only allowed before the
 * hash is instanciated.
 */
int comm_hash_set_size(uint32_t uSize)
{
  if (pCommHash != NULL)
    return -1;
  uCommHashSize= uSize;
  return 0;
}

// -----[ comm_hash_get_method ]-------------------------------------
uint8_t comm_hash_get_method()
{
  return uCommHashMethod;
}

// -----[ comm_hash_set_method ]-------------------------------------
int comm_hash_set_method(uint8_t uMethod)
{
  if (pCommHash != NULL)
    return -1;
  if ((uMethod != COMM_HASH_METHOD_STRING) && 
      (uMethod != COMM_HASH_METHOD_ZEBRA))
    return -1;
  uCommHashMethod= uMethod;
  switch (uCommHashMethod) {
  case COMM_HASH_METHOD_STRING:
    fCommHashCompute= _comm_hash_item_compute;
    break;
  case COMM_HASH_METHOD_ZEBRA:
    fCommHashCompute= _comm_hash_item_compute_zebra;
    break;
  default:
    abort();
  }
  return 0;
}

// -----[ comm_hash_content_for_each ]-------------------------------
/**
 *
 */
int _comm_hash_content_for_each(void * pItem, void * pContext)
{
  FILE * pStream= (FILE *) pContext;
  SCommunities * pCommunities= (SCommunities *) pItem;
  uint32_t uRefCnt= 0;

  uRefCnt= hash_info(pCommHash, pCommunities);

  fprintf(pStream, "%u\t", uRefCnt);
  comm_dump(pStream, pCommunities, 1);
  fprintf(pStream, "\n");
  return 0;
}

// -----[ comm_hash_content ]----------------------------------------
/**
 *
 */
void comm_hash_content(FILE * pStream)
{
  time_t tCurrentTime= time(NULL);

  _comm_hash_init();
  fprintf(pStream, "# C-BGP Global Communities repository content\n");
  fprintf(pStream, "# generated on %s", ctime(&tCurrentTime));
  fprintf(pStream, "# hash-size is %u\n", uCommHashSize);
  assert(!hash_for_each(pCommHash,
			_comm_hash_content_for_each,
			pStream));
}

// -----[ comm_hash_statistics_for_each ]----------------------------
/**
 *
 */
int _comm_hash_statistics_for_each(void * pItem, void * pContext)
{
  FILE * pStream= (FILE *) pContext;
  SPtrArray * pArray= (SPtrArray *) pItem;
  uint32_t uCntItems= 0;

  if (pArray != NULL)
    uCntItems= ptr_array_length(pArray);
  fprintf(pStream, "%u\n", uCntItems);
  return 0;
}

// -----[ comm_hash_statistics ]-------------------------------------
/**
 *
 */
void comm_hash_statistics(FILE * pStream)
{
  time_t tCurrentTime= time(NULL);

  _comm_hash_init();
  fprintf(pStream, "# C-BGP Global Communities repository statistics\n");
  fprintf(pStream, "# generated on %s", ctime(&tCurrentTime));
  fprintf(pStream, "# hash-size is %u\n", uCommHashSize);
  assert(!hash_for_each_key(pCommHash,
			    _comm_hash_statistics_for_each,
			    pStream));
}

/////////////////////////////////////////////////////////////////////
//
// INITIALIZATION AND FINALIZATION SECTION
//
/////////////////////////////////////////////////////////////////////

// -----[ _comm_hash_init ]-------------------------------------------
void _comm_hash_init()
{
  if (pCommHash == NULL) {
    pCommHash= hash_init(uCommHashSize,
			 0.5,
			 _comm_hash_item_compare,
			 _comm_hash_item_destroy,
			 fCommHashCompute);
    assert(pCommHash != NULL);
  }
}

// -----[ _comm_hash_destroy ]---------------------------------------
void _comm_hash_destroy()
{
  if (pCommHash != NULL)
    hash_destroy(&pCommHash);
}
