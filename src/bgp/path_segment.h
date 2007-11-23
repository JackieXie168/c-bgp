// ==================================================================
// @(#)path_segment.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 28/10/2003
// @lastdate 23/11/2007
// ==================================================================

#ifndef __BGP_PATH_SEGMENT_H__
#define __BGP_PATH_SEGMENT_H__

#include <libgds/log.h>
#include <libgds/types.h>

#include <stdio.h>

#include <bgp/types.h>

#define AS_PATH_SEGMENT_SET             1
#define AS_PATH_SEGMENT_SEQUENCE        2
#define AS_PATH_SEGMENT_CONFED_SET      3
#define AS_PATH_SEGMENT_CONFED_SEQUENCE 4

#define MAX_PATH_SEQUENCE_LENGTH 255

#ifdef __cplusplus
extern "C" {
#endif

  // ----- path_segment_create --------------------------------------
  SPathSegment * path_segment_create(uint8_t uType, uint8_t uLength);
  // ----- path_segment_destroy -------------------------------------
  void path_segment_destroy(SPathSegment ** ppSegment);
  // ----- path_segment_copy ----------------------------------------
  SPathSegment * path_segment_copy(SPathSegment * pSegment);
  // -----[ path_segment_to_string ]---------------------------------
  int path_segment_to_string(SPathSegment * pSegment,
			     uint8_t uReverse,
			     char * pcDst,
				  size_t tDstSize);
  // -----[ path_segment_from_string ]-------------------------------
  SPathSegment * path_segment_from_string(const char * pcPathSegment);
  // ---- path_segment_dump_string ---------------------------------
  char * path_segment_dump_string(SPathSegment * pSegment,
				  uint8_t uReverse);
  // ----- path_segment_dump ----------------------------------------
  void path_segment_dump(SLogStream * pStream,
			 SPathSegment * pSegment,
			 uint8_t uReverse);
  // ----- path_segment_add -----------------------------------------
  int path_segment_add(SPathSegment ** ppSegment,
		       uint16_t uAS);
  // ----- path_segment_contains ------------------------------------
  inline int path_segment_contains(SPathSegment * pSegment,
				   uint16_t uAS);
  // ----- path_segment_equals --------------------------------------
  inline int path_segment_equals(SPathSegment * pSegment1,
				 SPathSegment * pSegment2);
  // -----[ path_segment_cmp ]---------------------------------------
  int path_segment_cmp(SPathSegment * pSeg1, SPathSegment * pSeg2);
  // -----[ path_segment_remove_private ]----------------------------
  SPathSegment * path_segment_remove_private(SPathSegment * pSegment);

  // -----[ _path_segment_destroy ]----------------------------------
  void _path_segment_destroy();

#ifdef __cplusplus
}
#endif

#endif /* __BGP_PATH_SEGMENT_H__ */
