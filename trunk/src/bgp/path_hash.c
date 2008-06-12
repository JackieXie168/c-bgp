// ==================================================================
// @(#)path_hash.c
//
// This is the global AS-Path repository.
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 10/10/2005
// $Id: path_hash.c,v 1.9 2008-06-12 09:44:51 bqu Exp $
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <libgds/hash.h>
#include <libgds/hash_utils.h>
#include <libgds/log.h>

#include <bgp/path.h>
#include <bgp/path_hash.h>

// ---| Function prototypes |---
static uint32_t _path_hash_item_compute(const void * pPath, unsigned int uHashSize);

// ---| Private parameters |---
static hash_t * pPathHash= NULL;
static unsigned int uPathHashSize= 25000;
static uint8_t uPathHashMethod= PATH_HASH_METHOD_STRING;
static FHashCompute fPathHashCompute= _path_hash_item_compute;

// -----[ _path_hash_item_compute ]----------------------------------
/**
 * This is a helper function that computes the hash key of an
 * AS-Path. The function relies on a static buffer in order to avoid
 * frequent memory allocation.
 */
#define AS_PATH_STR_SIZE 1024
static uint32_t _path_hash_item_compute(const void * pPath, unsigned int uHashSize)
{
  char acPathStr1[AS_PATH_STR_SIZE];
  uint32_t tKey;
  assert(path_to_string((SBGPPath *) pPath, 1, acPathStr1, AS_PATH_STR_SIZE) >= 0);
  tKey= hash_utils_key_compute_string(acPathStr1, uPathHashSize) % uHashSize;
  return tKey;
}

// -----[ path_hash_item_compare ]-----------------------------------
static int _path_hash_item_compare(void * pPath1, void * pPath2,
				   unsigned int uEltSize)
{
  //return path_str_cmp(pPath1, pPath2);
  return path_cmp(pPath1, pPath2);
}

// -----[ path_hash_item_destroy ]-----------------------------------
static void _path_hash_item_destroy(void * pItem)
{
  SBGPPath * pPath= (SBGPPath *) pItem;
  path_destroy(&pPath);
}

// -----[ path_hash_add ]--------------------------------------------
/**
 *
 */
void * path_hash_add(SBGPPath * pPath)
{
  _path_hash_init();
  return hash_add(pPathHash, pPath);
}

// -----[ path_hash_get ]--------------------------------------------
/**
 *
 */
SBGPPath * path_hash_get(SBGPPath * pPath)
{
  _path_hash_init();
  return (SBGPPath *) hash_search(pPathHash, pPath);
}

// -----[ path_hash_remove ]-----------------------------------------
/**
 *
 */
int path_hash_remove(SBGPPath * pPath)
{
  int iResult;
  _path_hash_init();
  assert((iResult= hash_del(pPathHash, pPath)) != 0);
  return iResult;
}

// -----[ path_hash_get_size ]---------------------------------------
/**
 * Return the AS-Path hash size.
 */
uint32_t path_hash_get_size()
{
  return uPathHashSize;
}

// -----[ path_hash_set_size ]---------------------------------------
/**
 * Change the AS-Path hash size. This is only allowed before the hash
 * is instanciated.
 */
int path_hash_set_size(uint32_t uSize)
{
  if (pPathHash != NULL)
    return -1;
  uPathHashSize= uSize;
  return 0;
}

// -----[ path_hash_get_method ]-------------------------------------
uint8_t path_hash_get_method()
{
  return uPathHashMethod;
}

// -----[ path_hash_set_method ]-------------------------------------
int path_hash_set_method(uint8_t uMethod)
{
  if (pPathHash != NULL)
    return -1;
  if ((uMethod != PATH_HASH_METHOD_STRING) && 
      (uMethod != PATH_HASH_METHOD_ZEBRA))
    return -1;
  uPathHashMethod= uMethod;
  switch (uPathHashMethod) {
  case PATH_HASH_METHOD_STRING:
    fPathHashCompute= _path_hash_item_compute;
    break;
  case PATH_HASH_METHOD_ZEBRA:
    fPathHashCompute= path_hash_zebra;
    break;
  case PATH_HASH_METHOD_OAT:
    fPathHashCompute= path_hash_OAT;
    break;
  default:
    abort();
  }
  return 0;
}

// -----[ path_hash_content_for_each ]-------------------------------
/**
 *
 */
int _path_hash_content_for_each(void * pItem, void * pContext)
{
  SLogStream * pStream= (SLogStream *) pContext;
  SBGPPath * pPath= (SBGPPath *) pItem;
  uint32_t uRefCnt= 0;

  uRefCnt= hash_get_refcnt(pPathHash, pPath);

  log_printf(pStream, "%u\t", uRefCnt);
  path_dump(pStream, pPath, 1);
  log_printf(pStream, " (%p)", pPath);
  log_printf(pStream, "\n");
  return 0;
}

// -----[ path_hash_content ]----------------------------------------
/**
 *
 */
void path_hash_content(SLogStream * pStream)
{
  time_t tCurrentTime= time(NULL);

  _path_hash_init();
  log_printf(pStream, "# C-BGP Global AS-Path repository content\n");
  log_printf(pStream, "# generated on %s", ctime(&tCurrentTime));
  log_printf(pStream, "# hash-size is %u\n", uPathHashSize);
  assert(!hash_for_each(pPathHash,
			_path_hash_content_for_each,
			pStream));
}

// -----[ path_hash_statistics_for_each ]----------------------------
/**
 *
 */
int _path_hash_statistics_for_each(void * pItem, void * pContext)
{
  SLogStream * pStream= (SLogStream *) pContext;
  SPtrArray * pArray= (SPtrArray *) pItem;
  uint32_t uCntItems= 0;

  if (pArray != NULL)
    uCntItems= ptr_array_length(pArray);
  log_printf(pStream, "%u\n", uCntItems);
  return 0;
}

// -----[ path_hash_statistics ]-------------------------------------
/**
 *
 */
void path_hash_statistics(SLogStream * pStream)
{
  time_t tCurrentTime= time(NULL);

  _path_hash_init();
  log_printf(pStream, "# C-BGP Global AS-Path repository statistics\n");
  log_printf(pStream, "# generated on %s", ctime(&tCurrentTime));
  log_printf(pStream, "# hash-size is %u\n", uPathHashSize);
  assert(!hash_for_each_key(pPathHash,
			    _path_hash_statistics_for_each,
			    pStream));
}

/////////////////////////////////////////////////////////////////////
//
// INITIALIZATION AND FINALIZATION SECTION
//
/////////////////////////////////////////////////////////////////////

// -----[ _path_hash_init ]-------------------------------------------
void _path_hash_init()
{
  if (pPathHash == NULL) {
    pPathHash= hash_init(uPathHashSize,
			 0,
			 _path_hash_item_compare,
			 _path_hash_item_destroy,
			 fPathHashCompute);
    assert(pPathHash != NULL);
  }
}

// -----[ _path_hash_destroy ]---------------------------------------
void _path_hash_destroy()
{
  if (pPathHash != NULL)
    hash_destroy(&pPathHash);
}
