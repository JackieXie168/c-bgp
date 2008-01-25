// ==================================================================
// @(#)path.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 02/12/2002
// @lastdate 03/01/2008
// ==================================================================

#ifndef __BGP_PATH_H__
#define __BGP_PATH_H__

#include <stdio.h>

#include <libgds/log.h>
#include <libgds/types.h>

#include <bgp/types.h>
#include <util/regex.h>

#ifdef __cplusplus
extern "C" {
#endif

  // ----- path_create ----------------------------------------------
  SBGPPath * path_create();
  // ----- path_destroy ---------------------------------------------
  void path_destroy(SBGPPath ** ppPath);
  // ----- path_max_value -------------------------------------------
  SBGPPath * path_max_value();
  // ----- path_copy ------------------------------------------------
  SBGPPath * path_copy(SBGPPath * pPath);
  // ----- path_num_segments ----------------------------------------
  int path_num_segments(SBGPPath * pPath);
  // ----- path_length ----------------------------------------------
  int path_length(SBGPPath * pPath);
  // ----- path_add_segment -----------------------------------------
  int path_add_segment(SBGPPath * pPath, SPathSegment *pSegment);
  // ----- path_append ----------------------------------------------
  int path_append(SBGPPath ** ppPath, uint16_t uAS);
  // ----- path_contains --------------------------------------------
  int path_contains(SBGPPath * pPath, uint16_t uAS);
  // -----[ path_last_as ]-------------------------------------------
  int path_last_as(SBGPPath * pPath);
  // -----[ path_first_as ]------------------------------------------
  int path_first_as(SBGPPath * pPath);
  // -----[ path_to_string ]-----------------------------------------
  int path_to_string(SBGPPath * pPath, uint8_t uReverse,
		     char * pcDst, size_t tDstSize);
  // -----[ path_from_string ]---------------------------------------
  SBGPPath * path_from_string(const char * pcPath);
  // ----- path_dump ------------------------------------------------
  void path_dump(SLogStream * pStream, SBGPPath * pPath, uint8_t uReverse);
  // ----- path_hash ------------------------------------------------
  int path_hash(SBGPPath * pPath);
  // -----[ path_hash_zebra ]----------------------------------------
  uint32_t path_hash_zebra(const void * pItem, const uint32_t uHashSize);
  // -----[ path_hash_OAT ]--------------------------------------------
  uint32_t path_hash_OAT(const void * pItem, const uint32_t uHashSize);
  // ----- path_comparison ------------------------------------------
  int path_comparison(SBGPPath * path1, SBGPPath * path2);
  // ----- path_equals ----------------------------------------------
  int path_equals(SBGPPath * pPath1, SBGPPath * pPath2);
  // -----[ path_cmp ]-----------------------------------------------
  int path_cmp(SBGPPath * pPath1, SBGPPath * pPath2);
  // -----[ path_str_cmp ]-------------------------------------------
  int path_str_cmp(SBGPPath * pPath1, SBGPPath * pPath2);
  // ----- path_match -----------------------------------------------
  int path_match(SBGPPath * pPath, SRegEx * pRegEx);
  // -----[ path_remove_private ]------------------------------------
  void path_remove_private(SBGPPath * pPath);

  // -----[ _path_destroy ]------------------------------------------
  void _path_destroy();

#ifdef __cplusplus
}
#endif

#endif /* __BGP_PATH_H__ */
