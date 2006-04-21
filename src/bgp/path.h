// ==================================================================
// @(#)path.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 02/12/2002
// @lastdate 03/03/2006
// ==================================================================

#ifndef __PATH_H__
#define __PATH_H__

#include <stdio.h>

#include <libgds/log.h>
#include <libgds/types.h>

#include <bgp/types.h>

// ----- path_create ------------------------------------------------
extern SBGPPath * path_create();
// ----- path_destroy -----------------------------------------------
extern void path_destroy(SBGPPath ** ppPath);
// ----- path_addref ------------------------------------------------
//extern void path_addref(SBGPPath ** ppPath);
// ----- path_unref -------------------------------------------------
//extern void path_unref(SBGPPath ** ppPath);
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
extern void path_dump(SLogStream * pStream, SBGPPath * pPath,
		      uint8_t uReverse);
// ----- path_hash --------------------------------------------------
extern int path_hash(SBGPPath * pPath);
// -----[ path_hash_zebra ]------------------------------------------
extern uint32_t path_hash_zebra(void * pItem, uint32_t uHashSize);
// -----[ path_hash_OAT ]--------------------------------------------
extern uint32_t path_hash_OAT(void * pItem, uint32_t uHashSize);
// ----- path_equals ------------------------------------------------
extern int path_equals(SBGPPath * pPath1, SBGPPath * pPath2);
// ----- path_aggregate ---------------------------------------------
/*extern SBGPPath * path_aggregate(SBGPPath * apPaths[],
  unsigned int uNumPaths);*/
// ----- path_match -------------------------------------------------
int path_match(SBGPPath * pPath, int iArrayPathRegExPos);

extern void _path_test();

#endif
