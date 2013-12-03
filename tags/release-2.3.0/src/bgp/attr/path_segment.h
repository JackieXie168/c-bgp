// ==================================================================
// @(#)path_segment.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 28/10/2003
// $Id: path_segment.h,v 1.1 2009-03-24 13:41:26 bqu Exp $
// ==================================================================

#ifndef __BGP_PATH_SEGMENT_H__
#define __BGP_PATH_SEGMENT_H__

#include <libgds/stream.h>
#include <libgds/types.h>

#include <stdio.h>

#include <bgp/types.h>

#define MAX_PATH_SEQUENCE_LENGTH 255

#ifdef __cplusplus
extern "C" {
#endif

  // ----- path_segment_create --------------------------------------
  bgp_path_seg_t * path_segment_create(uint8_t type, uint8_t length);
  // ----- path_segment_destroy -------------------------------------
  void path_segment_destroy(bgp_path_seg_t ** seg_ref);
  // ----- path_segment_copy ----------------------------------------
  bgp_path_seg_t * path_segment_copy(const bgp_path_seg_t * seg);
  // -----[ path_segment_to_string ]---------------------------------
  int path_segment_to_string(const bgp_path_seg_t * seg, int reverse,
			     char * dst, size_t dst_size);
  // -----[ path_segment_from_string ]-------------------------------
  bgp_path_seg_t * path_segment_from_string(const char * seg_str);
  // ----- path_segment_dump ----------------------------------------
  void path_segment_dump(gds_stream_t * stream,
			 const bgp_path_seg_t * seg, 
			 int reverse);
  // ----- path_segment_add -----------------------------------------
  int path_segment_add(bgp_path_seg_t ** seg_ref, asn_t asn);
  // ----- path_segment_contains ------------------------------------
  int path_segment_contains(const bgp_path_seg_t * seg, asn_t asn);
  // ----- path_segment_equals --------------------------------------
  int path_segment_equals(const bgp_path_seg_t * seg1,
			  const bgp_path_seg_t * seg2);
  // -----[ path_segment_cmp ]---------------------------------------
  int path_segment_cmp(const bgp_path_seg_t * seg1,
		       const bgp_path_seg_t * seg2);
  // -----[ path_segment_remove_private ]----------------------------
  bgp_path_seg_t * path_segment_remove_private(bgp_path_seg_t * seg);

  // -----[ _path_segment_destroy ]----------------------------------
  void _path_segment_destroy();

#ifdef __cplusplus
}
#endif

#endif /* __BGP_PATH_SEGMENT_H__ */
