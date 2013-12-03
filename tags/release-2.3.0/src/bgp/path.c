// ==================================================================
// @(#)path.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 02/12/2002
// @lastdate 03/01/2008
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <libgds/log.h>
#include <libgds/memory.h>
#include <libgds/tokenizer.h>

#include <bgp/path.h>
#include <bgp/path_hash.h>
#include <bgp/path_segment.h>
#include <bgp/filter.h>

static STokenizer * pPathTokenizer= NULL;

// ----- _path_destroy_segment --------------------------------------
static void _path_destroy_segment(void * pItem)
{
  path_segment_destroy((SPathSegment **) pItem);
}

// ----- path_create ------------------------------------------------
/**
 * Creates an empty AS-Path.
 */
SBGPPath * path_create()
{
  SBGPPath * pNewPath;
#ifndef  __BGP_PATH_TYPE_TREE__
  pNewPath= (SBGPPath *) ptr_array_create(0, NULL,
					  _path_destroy_segment);
#else
  pNewPath= (SBGPPath *) MALLOC(sizeof(SBGPPath));
  pNewPath->pSegment= NULL;
  pNewPath->pPrevious= NULL;
#endif
  return pNewPath;
}

// ----- path_destroy -----------------------------------------------
/**
 * Destroy an existing AS-Path.
 */
void path_destroy(SBGPPath ** ppPath)
{
#ifndef __BGP_PATH_TYPE_TREE__
  ptr_array_destroy(ppPath);
#else
  if (*ppPath != NULL) {
    if ((*ppPath)->pPrevious != NULL)
      path_hash_remove((*ppPath)->pPrevious);
    if ((*ppPath)->pSegment != NULL)
      path_segment_destroy(&(*ppPath)->pSegment);
    *ppPath= NULL;
  }
#endif
}

// ----- path_max_value ---------------------------------------------
/**
  * Create a path and sets the first ASN value to the maximum
  */
SBGPPath * path_max_value()
{
  SBGPPath * path = path_create();

  path_append(&path, 65535U);
  
  return path;
} 

// ----- path_copy --------------------------------------------------
/**
 * Duplicate an existing AS-Path.
 */
SBGPPath * path_copy(SBGPPath * pPath)
{
  SBGPPath * pNewPath= NULL;
#ifndef __BGP_PATH_TYPE_TREE__
  int iIndex;
#endif

  if (pPath == NULL)
    return NULL;

#ifndef __BGP_PATH_TYPE_TREE__
  pNewPath= path_create();
  for (iIndex= 0; iIndex < path_num_segments(pPath); iIndex++)
    path_add_segment(pNewPath, path_segment_copy((SPathSegment *) 
						 pPath->data[iIndex]));
#else
  abort();
#endif
  return pNewPath;
}

// ----- path_num_segments ------------------------------------------
/**
 * Return the number of segments in the given AS-Path.
 */
int path_num_segments(SBGPPath * pPath)
{
  if (pPath != NULL) {
#ifndef __BGP_PATH_TYPE_TREE__
    return ptr_array_length((SPtrArray *) pPath);
#else
    return 1;
#endif
  }
  return 0;
}

// ----- path_length ------------------------------------------------
/**
 * Return the number of AS hops in the given AS-Path. The length of an
 * AS-SEQUENCE segment is equal to the number of ASs which compose
 * it. The length of an AS-SET is equal to 1.
 */
int path_length(SBGPPath * pPath)
{
  int iLength= 0;
#ifndef __BGP_PATH_TYPE_TREE__
  int iIndex;
  SPathSegment * pSegment;

  if (pPath == NULL)
    return 0;

  for (iIndex= 0; iIndex < path_num_segments(pPath); iIndex++) {
    pSegment= ((SPathSegment *) pPath->data[iIndex]);
    switch (pSegment->uType) {
    case AS_PATH_SEGMENT_SEQUENCE:
      iLength+= pSegment->uLength;
      break;
    case AS_PATH_SEGMENT_SET:
      iLength++;
      break;
    default:
      abort();
    }
  }
#else
  abort();
#endif
  return iLength;
}

// ----- path_add_segment -------------------------------------------
/**
 * Add the given segment to the given path.
 *
 * Return value:
 *   >= 0   in case of success
 *   -1     in case of failure
 */
int path_add_segment(SBGPPath * pPath, SPathSegment *pSegment)
{
#ifndef __BGP_PATH_TYPE_TREE__
    return ptr_array_append((SPtrArray *) pPath, pSegment);
#else
    abort();
#endif
}

// ----- path_append ------------------------------------------------
/**
 * Append a new AS to the given AS-Path. If the last segment of the
 * AS-Path is an AS-SEQUENCE, the new AS is simply added at the end of
 * this segment. If the last segment is an AS-SET, a new segment of
 * type AS-SEQUENCE which contains only the new AS is added at the end
 * of the AS-Path.
 *
 * Return value:
 *   length(new AS-Path)   in case of success
 *   -1                    in case of error
 */
int path_append(SBGPPath ** ppPath, uint16_t uAS)
{
  int iLength= path_num_segments(*ppPath);

#ifndef __BGP_PATH_TYPE_TREE__
  SPathSegment * pSegment;

  if (iLength <= 0) {
    pSegment= path_segment_create(AS_PATH_SEGMENT_SEQUENCE, 1);
    pSegment->auValue[0]= uAS;
    path_add_segment(*ppPath, pSegment);
  } else {
    pSegment= (SPathSegment *) (*ppPath)->data[iLength-1];
    switch (pSegment->uType) {
    case AS_PATH_SEGMENT_SEQUENCE:
      if (path_segment_add((SPathSegment **) &(*ppPath)->data[iLength-1],
uAS))
	return -1;
      break;
    case AS_PATH_SEGMENT_SET:
      pSegment= path_segment_create(AS_PATH_SEGMENT_SEQUENCE, 1);
      pSegment->auValue[0]= uAS;
      path_add_segment(*ppPath, pSegment);
      break;
    default:
      abort();
    }
  }
#else
  abort();
#endif
  return iLength;
}

// ----- path_contains ----------------------------------------------
/**
 * Test if the given AS-Path contains the given AS number.
 *
 * Return value:
 *   1  if the path contains the ASN
 *   0  otherwise
 */
int path_contains(SBGPPath * pPath, uint16_t uAS)
{
#ifndef __BGP_PATH_TYPE_TREE__
  unsigned int uIndex;

  if (pPath == NULL)
    return 0;

  for (uIndex= 0; uIndex < path_num_segments(pPath); uIndex++) {
    if (path_segment_contains((SPathSegment *) pPath->data[uIndex], uAS) != 0)
      return 1;
  }
#else
  abort();
#endif
  return 0;
}

// ----- path_at ----------------------------------------------------
/**
 * Return the AS number at the given position whatever the including
 * segment is.
 */
int path_at(SBGPPath * pPath, int iPosition, uint16_t uAS)
{
#ifndef __BGP_PATH_TYPE_TREE__
  int iIndex, iSegIndex;
  SPathSegment * pSegment;

  for (iIndex= 0; (iPosition > 0) &&
	 (iIndex < path_num_segments(pPath));
       iIndex++) {
    pSegment= (SPathSegment *) pPath->data[iIndex];
    switch (pSegment->uType) {
    case AS_PATH_SEGMENT_SEQUENCE:
      for (iSegIndex= 0; iSegIndex ; iSegIndex++) {
	if (iPosition == 0)
	  return pSegment->auValue[iSegIndex];
	iPosition++;
      }
      break;
    case AS_PATH_SEGMENT_SET:
      if (iPosition == 0)
	return pSegment->auValue[0];
      iPosition++;
      break;
    default:
      abort();
    }
  }
#else
  abort();
#endif
  return -1;
}

// -----[ path_last_as ]---------------------------------------------
/**
 * Return the last AS-number in the AS-Path.
 *
 * Note: this function does not work if the last segment in the
 * AS-Path is of type AS-SET since there is no ordering of the
 * AS-numbers in this case.
 */
int path_last_as(SBGPPath * pPath) {
#ifndef __BGP_PATH_TYPE_TREE__
  SPathSegment * pSegment;

  if (path_length(pPath) > 0) {
    pSegment= (SPathSegment *)
      pPath->data[ptr_array_length(pPath)-1];

    // Check that the segment is of type AS_SEQUENCE 
    assert(pSegment->uType == AS_PATH_SEGMENT_SEQUENCE);
    // Check that segment's size is larger than 0
    assert(pSegment->uLength > 0);

    return pSegment->auValue[pSegment->uLength-1];
  } else {
    return -1;
  }
#else
  abort();
#endif
}

// -----[ path_first_as ]--------------------------------------------
/**
 * Return the first AS-number in the AS-Path. This function can be
 * used to retrieve the origin AS.
 *
 * Note: this function does not work if the first segment in the
 * AS-Path is of type AS-SET since there is no ordering of the
 * AS-numbers in this case.
 */
int path_first_as(SBGPPath * pPath) {
#ifndef __BGP_PATH_TYPE_TREE__
  SPathSegment * pSegment;

  if (path_length(pPath) > 0) {
    pSegment= (SPathSegment *) pPath->data[0];

    // Check that the segment is of type AS_SEQUENCE 
    assert(pSegment->uType == AS_PATH_SEGMENT_SEQUENCE);
    // Check that segment's size is larger than 0
    assert(pSegment->uLength > 0);

    return pSegment->auValue[0];
  } else {
    return -1;
  }
#else
  abort();
#endif
}

// -----[ path_to_string ]-------------------------------------------
/**
 * Convert the given AS-Path to a string. The string memory MUST have
 * been allocated before. The function will not write outside of the
 * allocated buffer, based on the provided destination buffer size.
 *
 * Return value:
 *   -1    if the destination buffer is too small
 *   >= 0  number of characters written (without \0)
 *
 * Note: the function uses snprintf() in order to write into the
 * destination buffer. The return value of snprintf() is important. A
 * return value equal or larger that the maximum destination size
 * means that the output was truncated.
 */
int path_to_string(SBGPPath * pPath, uint8_t uReverse,
		   char * pcDst, size_t tDstSize)
{
#ifndef __BGP_PATH_TYPE_TREE__
  size_t tInitialSize= tDstSize;
  int iIndex;
  int iWritten= 0;
  int iNumSegments;

  if ((pPath == NULL) || (path_num_segments(pPath) == 0)) {
    if (tDstSize < 1)
      return -1;
    *pcDst= '\0';
    return 0;
  }

  if (uReverse) {
    iNumSegments= path_num_segments(pPath);
    for (iIndex= iNumSegments; iIndex > 0; iIndex--) {
      // Write space (if required)
      if (iIndex < iNumSegments) {
	iWritten= snprintf(pcDst, tDstSize, " ");
	if (iWritten >= tDstSize)
	  return -1;
	pcDst+= iWritten;
	tDstSize-= iWritten;
      }

      // Write AS-Path segment
      iWritten= path_segment_to_string((SPathSegment *) pPath->data[iIndex-1],
				       uReverse, pcDst, tDstSize);
      if (iWritten >= tDstSize)
	return -1;
      pcDst+= iWritten;
      tDstSize-= iWritten;
    }

  } else {
    for (iIndex= 0; iIndex < path_num_segments(pPath);
	 iIndex++) {
      // Write space (if required)
      if (iIndex > 0) {
	iWritten= snprintf(pcDst, tDstSize, " ");
	if (iWritten >= tDstSize)
	  return -1;
	pcDst+= iWritten;
	tDstSize-= iWritten;
      }
      // Write AS-Path segment
      iWritten= path_segment_to_string((SPathSegment *) pPath->data[iIndex],
				       uReverse, pcDst, tDstSize);
      if (iWritten >= tDstSize)
	return -1;
      pcDst+= iWritten;
      tDstSize-= iWritten;
    }
  }
  return tInitialSize-tDstSize;
#else
  abort();
#endif
}

// -----[ path_from_string ]-----------------------------------------
/**
 * Convert a string to an AS-Path.
 *
 * Return value:
 *   NULL  if the string does not contain a valid AS-Path
 *   new path otherwise
 */
SBGPPath * path_from_string(const char * pcPath)
{
  int iIndex;
  STokens * pPathTokens;
  SBGPPath * pPath= NULL;
  char * pcSegment;
  SPathSegment * pSegment;
  unsigned long ulASNum;

  if (pPathTokenizer == NULL)
    pPathTokenizer= tokenizer_create(" ", 0, "{[(", "}])");

  if (tokenizer_run(pPathTokenizer, (char *) pcPath) != TOKENIZER_SUCCESS)
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
      pSegment= path_segment_from_string(pcSegment);
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

// ----- path_match --------------------------------------------------------
/**
 *
 */
int path_match(SBGPPath * pPath, SRegEx * pRegEx)
{
  char acBuffer[1000];
  int iRet = 0;

  if (path_to_string(pPath, 1, acBuffer, sizeof(acBuffer)) < 0)
    abort();

  if (pRegEx != NULL) {
    if (strcmp(acBuffer, "null") != 0) {
      if (regex_search(pRegEx, acBuffer) > 0)
        iRet = 1;
      regex_reinit(pRegEx);
    }
  }

  return iRet;
}

// ----- path_dump --------------------------------------------------
/**
 * Dump the given AS-Path to the given ouput stream.
 */
void path_dump(SLogStream * pStream, SBGPPath * pPath, uint8_t uReverse)
{
#ifndef __BGP_PATH_TYPE_TREE__
  int iIndex;

  if ((pPath == NULL) || (path_num_segments(pPath) == 0)) {
    log_printf(pStream, "");
  } else {
    if (uReverse) {
      for (iIndex= path_num_segments(pPath); iIndex > 0;
	   iIndex--) {
	path_segment_dump(pStream, (SPathSegment *) pPath->data[iIndex-1],
			  uReverse);
	if (iIndex > 1)
	  log_printf(pStream, " ");
      }
    } else {
      for (iIndex= 0; iIndex < path_num_segments(pPath);
	   iIndex++) {
	if (iIndex > 0)
	  log_printf(pStream, " ");
	path_segment_dump(pStream, (SPathSegment *) pPath->data[iIndex],
			  uReverse);
      }
    }
  }
#else
  abort();
#endif
}

// ----- path_hash --------------------------------------------------
/**
 * Universal hash function for string keys (discussed in Sedgewick's
 * "Algorithms in C, 3rd edition") and adapted for AS-Paths.
 */
int path_hash(SBGPPath * pPath)
{
#ifndef __BGP_PATH_TYPE_TREE__
  int iHash, a = 31415, b = 27183, M = 65521;
  int iIndex, iSegIndex;
  SPathSegment * pSegment;

  if (pPath == NULL)
    return 0;

  iHash= 0;
  for (iIndex= 0; iIndex < path_num_segments(pPath); iIndex++) {
    pSegment= (SPathSegment *) pPath->data[iIndex];
    for (iSegIndex= 0; iSegIndex < pSegment->uLength; iSegIndex++) {
      iHash= (a*iHash+pSegment->auValue[iSegIndex])%M;
      a= a*b%(M-1);
    }
  }
  return iHash;
#else
  abort();
#endif
}

// -----[ path_hash_zebra ]------------------------------------------
/**
 * This is a helper function that computes the hash key of an
 * AS-Path. The function is based on Zebra's AS-Path hashing
 * function.
 */
uint32_t path_hash_zebra(const void * pItem, unsigned int uHashSize)
{
#ifndef __BGP_PATH_TYPE_TREE__
  SBGPPath * pPath= (SBGPPath *) pItem;
  unsigned int uKey= 0;
  uint32_t uIndex, uIndex2;
  SPathSegment * pSegment;

  for (uIndex= 0; uIndex < path_num_segments(pPath); uIndex++) {
    pSegment= (SPathSegment *) pPath->data[uIndex];
    // Note: segment type IDs are equal to those of Zebra
    //(1 AS_SET, 2 AS_SEQUENCE)
    uKey+= pSegment->uType;
    for (uIndex2= 0; uIndex2 < pSegment->uLength; uIndex2++) {
      uKey+= pSegment->auValue[uIndex2] & 255;
      uKey+= pSegment->auValue[uIndex2] >> 8;
    }
  }
  return uKey;
#else
  abort();
#endif
}

// -----[ path_hash_OAT ]--------------------------------------------
/**
 * Note: uHashSize must be a power of 2.
 */
uint32_t path_hash_OAT(const void * pItem, unsigned int uHashSize)
{
#ifndef __BGP_PATH_TYPE_TREE__
  uint32_t uHash= 0;
  SBGPPath * pPath= (SBGPPath *) pItem;
  uint32_t uIndex, uIndex2;
  SPathSegment * pSegment;

  for (uIndex= 0; uIndex < path_num_segments(pPath); uIndex++) {
    pSegment= (SPathSegment *) pPath->data[uIndex];
    // Note: segment type IDs are equal to those of Zebra
    //(1 AS_SET, 2 AS_SEQUENCE)

    uHash+= pSegment->uType;
    uHash+= (uHash << 10);
    uHash^= (uHash >> 6);
 
    for (uIndex2= 0; uIndex2 < pSegment->uLength; uIndex2++) {
      uHash+= pSegment->auValue[uIndex2] & 255;
      uHash+= (uHash << 10);
      uHash^= (uHash >> 6);
      uHash+= pSegment->auValue[uIndex2] >> 8;
      uHash+= (uHash << 10);
      uHash^= (uHash >> 6);
    }
  }
  uHash+= (uHash << 3);
  uHash^= (uHash >> 11);
  uHash+= (uHash << 15);
  return uHash % uHashSize;
#else
  abort();
#endif
}

// ----- path_comparison --------------------------------------------
/**
  * Do a comparison ASN value by ASN value between two paths.
  * It does *not* take into account the length of the as-path first.
  * if you want to do so, then first use path_equals.
  * But if you have two paths as the following:
  * 1) 22 23 24 25
  * 2) 22 23 24
  *The first one will be considered as greater because of his length but ...
  * 1) 25 24 23 22
  * 2) 24 23 22
  * in this case the first will be considered as greater because of the first ASN value.
  */
int path_comparison(SBGPPath * path1, SBGPPath * path2)
{
  uint16_t uLength;
  uint16_t uIndex;

  if (path_length(path1) < path_length(path2))
    uLength = path_length(path1);
  else
    uLength = path_length(path2);

  for (uIndex = 0; uIndex < uLength; uIndex++) {
    if (path_at(path1, uIndex, 0) < path_at(path2, uIndex, 0))
      return -1;
    else
      if (path_at(path1, uIndex, 0) > path_at(path2, uIndex, 0))
        return 1;
  }
  return path_equals(path1, path2);
}

// ----- path_equals ------------------------------------------------
/**
 * Test if two paths are equal. If they are equal, the function
 * returns 1, otherwize, the function returns 0.
 */
int path_equals(SBGPPath * pPath1, SBGPPath * pPath2)
{
#ifndef __BGP_PATH_TYPE_TREE__
  int iIndex;

  if (pPath1 == pPath2)
    return 1;

  if (path_num_segments(pPath1) != path_num_segments(pPath2))
    return 0;

  for (iIndex= 0; iIndex < path_num_segments(pPath1); iIndex++) {
    if (!path_segment_equals((SPathSegment *) pPath1->data[iIndex],
			     (SPathSegment *) pPath2->data[iIndex]))
      return 0;
  }

  return 1;
#else
  abort();
#endif
}

// -----[ path_cmp ]-------------------------------------------------
/**
 * Compare two AS-Paths. This function is aimed at sorting AS-Paths,
 * not at measuring their length for the decision process !
 *
 * Return value:
 *   -1  if path1 < path2
 *   0   if path1 == path2
 *   1   if path1 > path2
 */
int path_cmp(SBGPPath * pPath1, SBGPPath * pPath2)
{
  unsigned int uIndex;
  int iCmp;

  // Null paths are equal
  if (pPath1 == pPath2)
    return 0;

  // Null and empty paths are equal
  if (((pPath1 == NULL) && (path_num_segments(pPath2) == 0)) ||
      ((pPath2 == NULL) && (path_num_segments(pPath1) == 0)))
    return 0;

  if (pPath1 == NULL)
    return -1;
  if (pPath2 == NULL)
    return 1;
  
  // Longest is first
  if (path_num_segments(pPath1) < path_num_segments(pPath2))
    return -1;
  else if (path_num_segments(pPath1) > path_num_segments(pPath2))
    return 1;

  // Equal size paths, inspect individual segments
  for (uIndex= 0; uIndex < path_num_segments(pPath1); uIndex++) {
    iCmp= path_segment_cmp((SPathSegment *) pPath1->data[uIndex],
			   (SPathSegment *) pPath2->data[uIndex]);
    if (iCmp != 0)
      return iCmp;
  }

  return 0;
}

// -----[ path_str_cmp ]---------------------------------------------
int path_str_cmp(SBGPPath * pPath1, SBGPPath * pPath2)
{
#define AS_PATH_STR_SIZE 1024
  char acPathStr1[AS_PATH_STR_SIZE];
  char acPathStr2[AS_PATH_STR_SIZE];

  // If paths pointers are equal, no need to compare their content.
  if (pPath1 == pPath2)
    return 0;

  assert(path_to_string(pPath1, 1, acPathStr1, AS_PATH_STR_SIZE) >= 0);
  assert(path_to_string(pPath2, 1, acPathStr2, AS_PATH_STR_SIZE) >= 0);
   
  return strcmp(acPathStr1, acPathStr2);
}

// -----[ path_remove_private ]--------------------------------------
void path_remove_private(SBGPPath * pPath)
{
  unsigned int uIndex;
  SPathSegment * pSegment;
  SPathSegment * pNewSegment;

  uIndex= 0;
  while (uIndex < path_num_segments(pPath)) {
    pSegment= (SPathSegment *) pPath->data[uIndex];
    pNewSegment= path_segment_remove_private(pSegment);
    if (pSegment != pNewSegment) {
      if (pNewSegment == NULL) {
	ptr_array_remove_at(pPath, uIndex);
	continue;
      } else {
	pPath->data[uIndex]= pNewSegment;
      }
    }
    uIndex++;
  }
}

// -----[ _path_destroy ]--------------------------------------------
void _path_destroy()
{
  tokenizer_destroy(&pPathTokenizer);
}




