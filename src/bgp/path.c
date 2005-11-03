// ==================================================================
// @(#)path.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 02/12/2002
// @lastdate 17/10/2005
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <libgds/log.h>
#include <libgds/memory.h>

#include <bgp/path.h>
#include <bgp/path_hash.h>
#include <bgp/path_segment.h>
#include <bgp/filter.h>

// ----- path_destroy_segment ---------------------------------------
void path_destroy_segment(void * pItem)
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
					  path_destroy_segment);
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

// ----- path_copy --------------------------------------------------
/**
 * Duplicate an existing AS-Path.
 */
SBGPPath * path_copy(SBGPPath * pPath)
{
  SBGPPath * pNewPath= NULL;

  if (pPath == NULL)
    return NULL;

#ifndef __BGP_PATH_TYPE_TREE__
  int iIndex;

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
      if (path_segment_add((SPathSegment **) &(*ppPath)->data[iLength-1], uAS))
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
 */
int path_contains(SBGPPath * pPath, uint16_t uAS)
{
#ifndef __BGP_PATH_TYPE_TREE__
  int iIndex;
  int iResult;

  for (iIndex= 0; iIndex < path_num_segments(pPath); iIndex++) {
    iResult= path_segment_contains((SPathSegment *) pPath->data[iIndex], uAS);
    if (iResult >= 0)
      return iResult;
  }
#else
  abort();
#endif
  return -1;
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

// -----[ path_to_string ]-------------------------------------------
/**
 * Convert the given AS-Path to a string. The string memory MUST have
 * been allocated before. The function will not write outside of the
 * allocated buffer, based on the provided destination buffer size.
 *
 * Return value:
 *   The function returns a valid pointer if the AS-Path could be
 *   written in the provided buffer. The returned pointer is the first
 *   character that follows the conversion. If there was not enough
 *   space in the buffer to write the AS-Path, then NULL is returned.
 */
int path_to_string(SBGPPath * pPath, uint8_t uReverse,
		   char * pcDst, size_t tDstSize)
{
#ifndef __BGP_PATH_TYPE_TREE__
  int iIndex;
  int iWritten;
  size_t tInitialDstSize= tDstSize;
  int iNumSegments;

  if (pPath == NULL) {
    iWritten= snprintf(pcDst, tDstSize, "null");
    if (iWritten >= tDstSize)
      return tDstSize;
    pcDst+= iWritten;
    tDstSize-= iWritten;
    return (tInitialDstSize-tDstSize);
  }

  if (uReverse) {
    iNumSegments= path_num_segments(pPath); 
    for (iIndex= iNumSegments; iIndex > 0; iIndex--) {
      // Write space (if required)
      if (iIndex < iNumSegments) {
	iWritten= snprintf(pcDst, tDstSize, " ");
	if (iWritten >= tDstSize)
	  return tInitialDstSize;
	pcDst+= iWritten;
	tDstSize-= iWritten;
      }
      // Write AS-Path segment
      iWritten= path_segment_to_string((SPathSegment *) pPath->data[iIndex-1],
				       uReverse, pcDst, tDstSize);
      if (iWritten >= tDstSize)
	return tInitialDstSize;
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
	  return tInitialDstSize;
	pcDst+= iWritten;
	tDstSize-= iWritten;
      }
      // Write AS-Path segment
      iWritten= path_segment_to_string((SPathSegment *) pPath->data[iIndex],
				       uReverse, pcDst, tDstSize);
      if (iWritten >= tDstSize)
	return tInitialDstSize;
      pcDst+= iWritten;
      tDstSize-= iWritten;
    }
  }
  return (tInitialDstSize-tDstSize);
#else
  abort();
#endif
}

extern SPtrArray * paPathExpr;
// ----- path_match --------------------------------------------------------
/**
 *
 *
 */
int path_match(SBGPPath * pPath, int iArrayPathRegExPos)
{
  SPathMatch * pPathMatch = NULL; 
  char * pcPathDump= path_dump_string(pPath, 1);
  int iRet = 0;

  ptr_array_get_at(paPathExpr, iArrayPathRegExPos, &pPathMatch);
  if (pPathMatch != NULL) {
    if (strcmp(pcPathDump, "null") != 0) {
      if (regex_search(pPathMatch->pRegEx, pcPathDump) > 0)
        iRet = 1;
      regex_reinit(pPathMatch->pRegEx);
    }
  }

  FREE(pcPathDump);
  return iRet;
}

// ----- path_dump_string --------------------------------------------------
/**
 * Dump the given AS-Path to the given ouput stream.
 *
 * NOTE FROM BQU: this function is DANGEROUS and must be updated in
 * order check that the AS-Path length is not too long. Otherwise,
 * anything can happen!!! I would suggest to replace any use of this
 * function by the above path_to_string() function.
 */
char * path_dump_string(SBGPPath * pPath, uint8_t uReverse)
{
#ifndef __BGP_PATH_TYPE_TREE__
  int iIndex;
  char * cPath = MALLOC(255), * cCharTmp;
  uint8_t icPathPtr = 0;

  if (pPath == NULL) {
    strcpy(cPath, "null");
  } else {
    if (uReverse) {
      for (iIndex= path_num_segments(pPath); iIndex > 0;
	   iIndex--) {
	
	cCharTmp = path_segment_dump_string((SPathSegment *) pPath->data[iIndex-1],
			  uReverse);
	strcpy(cPath+icPathPtr, cCharTmp);
	icPathPtr += strlen(cCharTmp);
	FREE(cCharTmp);
	
	if (iIndex > 1)
	  strcpy(cPath+icPathPtr++, " ");
      }
    } else {
      for (iIndex= 0; iIndex < path_num_segments(pPath);
	   iIndex++) {
	if (iIndex > 0)
	  strcpy(cPath+icPathPtr++, " ");
	cCharTmp = path_segment_dump_string((SPathSegment *) pPath->data[iIndex],
			  uReverse);
	strcpy(cPath, cCharTmp);
	icPathPtr += strlen(cCharTmp);
	FREE(cCharTmp);
      }
    }
  }
  return cPath;
#else
  abort();
#endif
}


// ----- path_dump --------------------------------------------------
/**
 * Dump the given AS-Path to the given ouput stream.
 */
void path_dump(FILE * pStream, SBGPPath * pPath, uint8_t uReverse)
{
#ifndef __BGP_PATH_TYPE_TREE__
  int iIndex;

  if (pPath == NULL) {
    fprintf(pStream, "null");
  } else {
    if (uReverse) {
      for (iIndex= path_num_segments(pPath); iIndex > 0;
	   iIndex--) {
	path_segment_dump(pStream, (SPathSegment *) pPath->data[iIndex-1],
			  uReverse);
	if (iIndex > 1)
	  fprintf(pStream, " ");
      }
    } else {
      for (iIndex= 0; iIndex < path_num_segments(pPath);
	   iIndex++) {
	if (iIndex > 0)
	  fprintf(pStream, " ");
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
uint32_t path_hash_zebra(void * pItem, uint32_t uHashSize)
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

// ----- path_aggregate ---------------------------------------------
/**
 * Simple AS-Path aggregation:
 *
 * Create a path with a single segment of type AS-SET which contains
 * every AS number found in the input paths.
 */
/*SBGPPath * path_aggregate(SBGPPath * apPaths[], unsigned int uNumPaths)
{
  SBGPPath * pAggrPath= path_create();
  int iPathIndex, iSegmentIndex, iIndex;
  SUInt16Array * pASNumArray= uint16_array_create(ARRAY_OPTION_SORTED|
						  ARRAY_OPTION_UNIQUE);
  SPathSegment * pSegment;
  
  for (iPathIndex= 0; iPathIndex < uNumPaths; iPathIndex++) {
    for (iSegmentIndex= 0; iSegmentIndex <
	   path_num_segments(apPaths[iPathIndex]); iSegmentIndex++) {
      pSegment= (SPathSegment *) apPaths[iPathIndex]->data[iSegmentIndex];
      for (iIndex= 0; iIndex < pSegment->uLength; iIndex++)
	_array_add((SArray *) pASNumArray, &pSegment->auValue[iIndex]);
    }
  }

  pSegment= path_segment_create(AS_PATH_SEGMENT_SET, 0);
  for (iIndex= 0; iIndex < _array_length((SArray *) pASNumArray);
       iIndex++)
    path_segment_add(&pSegment, pASNumArray->data[iIndex]);
  path_add_segment(pAggrPath, pSegment);
  uint16_array_destroy(&pASNumArray);
  return pAggrPath;
  }*/

/////////////////////////////////////////////////////////////////////
// TEST & VALIDATION SECTION
/////////////////////////////////////////////////////////////////////

void _path_test()
{
  SPathSegment * pSegment= path_segment_create(AS_PATH_SEGMENT_SET, 0);
  SBGPPath * pPath= path_create();
  SBGPPath * pPath2= path_create();
  //SBGPPath * pAggrPath= NULL;
  SBGPPath * apPaths[2];

  path_segment_add(&pSegment, 4);
  path_segment_add(&pSegment, 5);
  path_segment_add(&pSegment, 6);
  path_add_segment(pPath, pSegment);
  path_dump(stdout, pPath, 1); fprintf(stdout, ": length=%d\n", path_length(pPath));
  path_append(&pPath, 1);
  path_dump(stdout, pPath, 1); fprintf(stdout, ": length=%d\n", path_length(pPath));
  path_append(&pPath, 2);
  path_dump(stdout, pPath, 1); fprintf(stdout, ": length=%d\n", path_length(pPath));
  path_append(&pPath, 3);
  path_dump(stdout, pPath, 1); fprintf(stdout, ": length=%d\n",
				       path_length(pPath));

  path_append(&pPath2, 1);
  path_append(&pPath2, 3);
  path_append(&pPath2, 7);
  path_append(&pPath2, 6);
  path_dump(stdout, pPath2, 1); fprintf(stdout, "\n");

  apPaths[0]= pPath;
  apPaths[1]= pPath2;
  /*pAggrPath= path_aggregate(apPaths, 2),
    path_dump(stdout, pAggrPath, 1); fprintf(stdout, "\n");*/
  path_destroy(&pPath);
  path_destroy(&pPath2);
  //path_destroy(&pAggrPath);
}



