// ==================================================================
// @(#)path.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 02/12/2002
// @lastdate 23/11/2003
// ==================================================================

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <libgds/log.h>
#include <libgds/memory.h>

#include <bgp/path.h>
#include <bgp/path_segment.h>

// ----- path_destroy_segment ---------------------------------------
void path_destroy_segment(void * pItem)
{
  path_segment_destroy((SPathSegment **) pItem);
}

// ----- path_create ------------------------------------------------
/**
 * Creates an empty AS-Path.
 */
SPath * path_create()
{
  return (SPath *) ptr_array_create(0, NULL, path_destroy_segment);
}

// ----- path_destroy -----------------------------------------------
/**
 * Destroy an existing AS-Path.
 */
void path_destroy(SPath ** ppPath)
{
  ptr_array_destroy((SPtrArray **) ppPath);
}

// ----- path_copy --------------------------------------------------
/**
 * Duplicate an existing AS-Path.
 */
SPath * path_copy(SPath * pPath)
{
  SPath * pNewPath= NULL;
  int iIndex;

  if (pPath != NULL) {
    pNewPath= path_create();
    for (iIndex= 0; iIndex < path_num_segments(pPath); iIndex++)
      path_add_segment(pNewPath, path_segment_copy((SPathSegment *) 
						pPath->data[iIndex]));
  }
  return pNewPath;
}

// ----- path_num_segments ------------------------------------------
/**
 * Return the number of segments in the given AS-Path.
 */
int path_num_segments(SPath * pPath)
{
  if (pPath != NULL)
    return ptr_array_length((SPtrArray *) pPath);
  return 0;
}

// ----- path_length ------------------------------------------------
/**
 * Return the number of AS hops in the given AS-Path. The length of an
 * AS-SEQUENCE segment is equal to the number of ASs which compose
 * it. The length of an AS-SET is equal to 1.
 */
int path_length(SPath * pPath)
{
  int iIndex;
  int iLength= 0;
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
  return iLength;
}

// ----- path_add_segment -------------------------------------------
/**
 * Add the given segment to the given path.
 */
int path_add_segment(SPath * pPath, SPathSegment *pSegment)
{
  return ptr_array_append((SPtrArray *) pPath, pSegment);
}

// ----- path_append ------------------------------------------------
/**
 * Append a new AS to the given AS-Path. If the last segment of the
 * AS-Path is an AS-SEQUENCE, the new AS is simply added at the end of
 * this segment. If the last segment is an AS-SET, a new segment of
 * type AS-SEQUENCE which contains only the new AS is added at the end
 * of the AS-Path.
 */
int path_append(SPath ** ppPath, uint16_t uAS)
{
  int iLength= path_num_segments(*ppPath);
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
  return 0;
}

// ----- path_contains ----------------------------------------------
/**
 * Test if the given AS-Path contains the given AS number.
 */
int path_contains(SPath * pPath, uint16_t uAS)
{
  int iIndex;
  int iResult;

  for (iIndex= 0; iIndex < path_num_segments(pPath); iIndex++) {
    iResult= path_segment_contains((SPathSegment *) pPath->data[iIndex], uAS);
    if (iResult >= 0)
      return iResult;
  }
  return -1;
}

// ----- path_at ----------------------------------------------------
/**
 * Return the AS number at the given position whatever the including
 * segment is.
 */
int path_at(SPath * pPath, int iPosition, uint16_t uAS)
{
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
  return -1;
}

// ----- path_dump_string --------------------------------------------------
/**
 * Dump the given AS-Path to the given ouput stream.
 */
char * path_dump_string(SPath * pPath, uint8_t uReverse)
{
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
	strcpy(cPath, cCharTmp);
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
}


// ----- path_dump --------------------------------------------------
/**
 * Dump the given AS-Path to the given ouput stream.
 */
void path_dump(FILE * pStream, SPath * pPath, uint8_t uReverse)
{
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
}

// ----- path_hash --------------------------------------------------
/**
 * Universal hash function for string keys (discussed in Sedgewick's
 * "Algorithms in C, 3rd edition") and adapted for AS-Paths.
 */
int path_hash(SPath * pPath)
{
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
}

// ----- path_equals ------------------------------------------------
/**
 * Test if two paths are equal. If they are equal, the function
 * returns 1, otherwize, the function returns 0.
 */
int path_equals(SPath * pPath1, SPath * pPath2)
{
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
}

// ----- path_aggregate ---------------------------------------------
/**
 * Simple AS-Path aggregation:
 *
 * Create a path with a single segment of type AS-SET which contains
 * every AS number found in the input paths.
 */
SPath * path_aggregate(SPath * apPaths[], unsigned int uNumPaths)
{
  SPath * pAggrPath= path_create();
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
}

/////////////////////////////////////////////////////////////////////
// TEST & VALIDATION SECTION
/////////////////////////////////////////////////////////////////////

void _path_test()
{
  SPathSegment * pSegment= path_segment_create(AS_PATH_SEGMENT_SET, 0);
  SPath * pPath= path_create();
  SPath * pPath2= path_create();
  SPath * pAggrPath= NULL;
  SPath * apPaths[2];

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
  pAggrPath= path_aggregate(apPaths, 2),
  path_dump(stdout, pAggrPath, 1); fprintf(stdout, "\n");
  path_destroy(&pPath);
  path_destroy(&pPath2);
  path_destroy(&pAggrPath);
}



