// ==================================================================
// @(#)path.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 02/12/2002
// @lastdate 13/08/2004
// ==================================================================

#ifndef __PATH_H__
#define __PATH_H__

#include <stdio.h>

#include <libgds/array.h>
#include <libgds/types.h>

#include <bgp/path_segment.h>

typedef SPtrArray SPath;

// ----- path_create ------------------------------------------------
extern SPath * path_create();
// ----- path_destroy -----------------------------------------------
extern void path_destroy(SPath ** ppPath);
// ----- path_copy --------------------------------------------------
extern SPath * path_copy(SPath * pPath);
// ----- path_num_segments ------------------------------------------
extern int path_num_segments(SPath * pPath);
// ----- path_length ------------------------------------------------
extern int path_length(SPath * pPath);
// ----- path_add_segment -------------------------------------------
extern int path_add_segment(SPath * pPath, SPathSegment *pSegment);
// ----- path_append ------------------------------------------------
extern int path_append(SPath ** ppPath, uint16_t uAS);
// ----- path_contains ----------------------------------------------
extern int path_contains(SPath * pPath, uint16_t uAS);
// ----- path_dump_string -------------------------------------------
char * path_dump_string(SPath * pPath, uint8_t uReverse);
// ----- path_dump --------------------------------------------------
extern void path_dump(FILE * pStream, SPath * pPath,
		      uint8_t uReverse);
// ----- path_hash --------------------------------------------------
extern int path_hash(SPath * pPath);
// ----- path_equals ------------------------------------------------
extern int path_equals(SPath * pPath1, SPath * pPath2);
// ----- path_aggregate ---------------------------------------------
extern SPath * path_aggregate(SPath * apPaths[],
			      unsigned int uNumPaths);
// ----- path_match -------------------------------------------------
int path_match(SPath * pPath, int iArrayPathRegExPos);

extern void _path_test();

#endif
