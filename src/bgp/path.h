// ==================================================================
// @(#)path.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 02/12/2002
// @lastdate 17/10/2005
// ==================================================================

#ifndef __PATH_H__
#define __PATH_H__

#include <stdio.h>

#include <libgds/array.h>
#include <libgds/types.h>

#include <bgp/path_segment.h>

// -----[ AS-Paths / AS-Trees ]--------------------------------------
/** 
 * There are two types of AS-Paths supported so far. The first one is
 * the simplest and contains the list of segments composing the
 * path. The second type can be used to reduce the memory consumption
 * and is composed of a segment and a previous AS-Path (called the
 * "tail") of the path.
 */
#ifndef __BGP_PATH_TYPE_TREE__
typedef SPtrArray SBGPPath;
#else
struct TBGPPath {
  SPathSegment * pSegment;
  struct TBGPPath * pPrevious;
};
typedef struct TBGPPath SBGPPath;
#endif

// ----- path_create ------------------------------------------------
extern SBGPPath * path_create();
// ----- path_destroy -----------------------------------------------
extern void path_destroy(SBGPPath ** ppPath);
// ----- path_copy --------------------------------------------------
extern SBGPPath * path_copy(SBGPPath * pPath);
// ----- path_num_segments ------------------------------------------
extern int path_num_segments(SBGPPath * pPath);
// ----- path_length ------------------------------------------------
extern int path_length(SBGPPath * pPath);
// ----- path_add_segment -------------------------------------------
extern int path_add_segment(SBGPPath * pPath, SPathSegment *pSegment);
// ----- path_append ------------------------------------------------
extern int path_append(SBGPPath ** ppPath, uint16_t uAS);
// ----- path_contains ----------------------------------------------
extern int path_contains(SBGPPath * pPath, uint16_t uAS);
// -----[ path_last_as ]---------------------------------------------
extern int path_last_as(SBGPPath * pPath);
// -----[ path_to_string ]-------------------------------------------
extern int path_to_string(SBGPPath * pPath, uint8_t uReverse,
			  char * pcDst, size_t tDstSize);
// ----- path_dump_string -------------------------------------------
char * path_dump_string(SBGPPath * pPath, uint8_t uReverse);
// ----- path_dump --------------------------------------------------
extern void path_dump(FILE * pStream, SBGPPath * pPath,
		      uint8_t uReverse);
// ----- path_hash --------------------------------------------------
extern int path_hash(SBGPPath * pPath);
// -----[ path_hash_zebra ]------------------------------------------
extern uint32_t path_hash_zebra(void * pItem, uint32_t uHashSize);
// ----- path_equals ------------------------------------------------
extern int path_equals(SBGPPath * pPath1, SBGPPath * pPath2);
// ----- path_aggregate ---------------------------------------------
/*extern SBGPPath * path_aggregate(SBGPPath * apPaths[],
  unsigned int uNumPaths);*/
// ----- path_match -------------------------------------------------
int path_match(SBGPPath * pPath, int iArrayPathRegExPos);

extern void _path_test();

#endif
