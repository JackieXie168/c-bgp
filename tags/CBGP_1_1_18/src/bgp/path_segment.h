// ==================================================================
// @(#)path_segment.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 28/10/2003
// @lastdate 28/10/2003
// ==================================================================

#ifndef __PATH_SEGMENT_H__
#define __PATH_SEGMENT_H__

#include <libgds/types.h>

#include <stdio.h>

#define AS_PATH_SEGMENT_SET 1
#define AS_PATH_SEGMENT_SEQUENCE 2

typedef struct {
  uint8_t uType;       /* Segment type */
  uint8_t uLength;     /* Number of ASs in the value field */
  uint16_t auValue[0]; /* One or more AS numbers */
} SPathSegment;

// ----- path_segment_create ----------------------------------------
/**
 * Create an AS-Path segment of the given type and length.
 */
extern SPathSegment * path_segment_create(uint8_t uType,
					  uint8_t uLength);

// ----- path_segment_destroy ---------------------------------------
/**
 * Destroy an AS-Path segment.
 */
extern void path_segment_destroy(SPathSegment ** ppSegment);

// ----- path_segment_copy ------------------------------------------
/**
 * Duplicate an existing AS-Path segment.
 */
extern SPathSegment * path_segment_copy(SPathSegment * pSegment);

// ---- path_segment_dump_string -----------------------------------
char * path_segment_dump_string(SPathSegment * pSegment,
		       uint8_t uReverse);
// ----- path_segment_dump ------------------------------------------
/**
 * Dump an AS-Path segment.
 */
extern void path_segment_dump(FILE * pStream,
			      SPathSegment * pSegment,
			      uint8_t uReverse);

// ----- path_segment_resize ----------------------------------------
/**
 * Resizes an existing segment.
 */
extern inline void path_segment_resize(SPathSegment ** ppSegment,
				       uint8_t uNewLength);

// ----- path_segment_add -------------------------------------------
/**
 * Add a new AS number to the given segment. Since the size of the
 * segment will be changed by this function, the segment's location in
 * memory can change. Thus, the pointer to the segment can change !!!
 *
 * The segment's size is limited to 255 AS numbers. If the segment can
 * not hold a additional AS number, the function returns
 * -1. Otherwize, the new AS number is added and the function returns
 * 0.
 */
extern int path_segment_add(SPathSegment ** ppSegment,
			    uint16_t uAS);

// ----- path_segment_contains --------------------------------------
/**
 * Test if the given segment contains the given AS number. If the AS
 * number was found, the function returns 0, else it returns the AS
 * number's position in the segment.
 */
extern inline int path_segment_contains(SPathSegment * pSegment,
					uint16_t uAS);

// ----- path_segment_equals ----------------------------------------
/**
 * Test if two segments are equal. If they are equal, the function
 * returns 1 otherwize, the function returns 0.
 */
extern inline int path_segment_equals(SPathSegment * pSegment1,
				      SPathSegment * pSegment2);

#endif
