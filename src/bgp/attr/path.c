// ==================================================================
// @(#)path.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 02/12/2002
// $Id: path.c,v 1.1 2009-03-24 13:41:26 bqu Exp $
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <libgds/stream.h>
#include <libgds/memory.h>
#include <libgds/tokenizer.h>

#include <bgp/attr/path.h>
#include <bgp/attr/path_hash.h>
#include <bgp/attr/path_segment.h>
#include <bgp/filter/filter.h>

static gds_tokenizer_t * path_tokenizer= NULL;

#define _path_num_segments(P) ((P) == NULL?0:ptr_array_length(P))
#define _path_segment_at(P, I) (bgp_path_seg_t *) (P)->data[(I)]
#define _path_segment_ref_at(P, I) (bgp_path_seg_t **) &((P)->data[(I)])
#define _path_segment_set(P, I, V) (P)->data[(I)]= (V)

// ----- _path_destroy_segment --------------------------------------
static void _path_destroy_segment(void * item, const void * ctx)
{
  path_segment_destroy((bgp_path_seg_t **) item);
}

// ----- path_create ------------------------------------------------
/**
 * Creates an empty AS-Path.
 */
bgp_path_t * path_create()
{
  return (bgp_path_t *) ptr_array_create(0, NULL,
					 _path_destroy_segment, NULL);
}

// ----- path_destroy -----------------------------------------------
/**
 * Destroy an existing AS-Path.
 */
void path_destroy(bgp_path_t ** ppath)
{
  ptr_array_destroy(ppath);
}

// ----- path_max_value ---------------------------------------------
/**
  * Create a path and sets the first ASN value to the maximum.
  *
  * This is only used in experimental WALTON.
  */
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
bgp_path_t * path_max_value()
{
  bgp_path_t * path = path_create();

  path_append(&path, MAX_AS);
  
  return path;
} 
#endif

// ----- path_copy --------------------------------------------------
/**
 * Duplicate an existing AS-Path.
 */
bgp_path_t * path_copy(bgp_path_t * path)
{
  bgp_path_t * new_path= NULL;
  unsigned int i;

  if (path == NULL)
    return NULL;

  new_path= path_create();
  for (i= 0; i < _path_num_segments(path); i++)
    path_add_segment(new_path, path_segment_copy(_path_segment_at(path, i)));
  return new_path;
}

// ----- path_num_segments ------------------------------------------
/**
 * Return the number of segments in the given AS-Path.
 */
int path_num_segments(const bgp_path_t * path)
{
  return _path_num_segments(path);
}


// ----- path_segment_at ------------------------------------------
bgp_path_seg_t * path_segment_at(bgp_path_t * path, int index)
{
  assert(index >= 0);
  if (index > _path_num_segments(path))
    return 0;
  return _path_segment_at(path, index);
}

// ----- path_length ------------------------------------------------
/**
 * Return the number of AS hops in the given AS-Path. The length of an
 * AS-SEQUENCE segment is equal to the number of ASs which compose
 * it. The length of an AS-SET is equal to 1.
 */
int path_length(bgp_path_t * path)
{
  int length= 0;
  unsigned int index;
  bgp_path_seg_t * seg;

  if (path == NULL)
    return 0;

  for (index= 0; index < _path_num_segments(path); index++) {
    seg= _path_segment_at(path, index);
    switch (seg->type) {
    case AS_PATH_SEGMENT_SEQUENCE:
      length+= seg->length;
      break;
    case AS_PATH_SEGMENT_SET:
      length++;
      break;
    default:
      abort();
    }
  }
  return length;
}

// ----- path_add_segment -------------------------------------------
/**
 * Add the given segment to the given path.
 *
 * Return value:
 *   >= 0   in case of success
 *   -1     in case of failure
 */
int path_add_segment(bgp_path_t * path, bgp_path_seg_t * seg)
{
  return ptr_array_append(path, seg);
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
int path_append(bgp_path_t ** ppath, asn_t asn)
{
  int length= path_num_segments(*ppath);

  bgp_path_seg_t * seg;

  if (length <= 0) {
    seg= path_segment_create(AS_PATH_SEGMENT_SEQUENCE, 1);
    seg->asns[0]= asn;
    path_add_segment(*ppath, seg);
  } else {

    // Find last segment.
    // - if it is a SEQUENCE, append.
    // - if it is a SET, create new segment.
    seg= _path_segment_at(*ppath, length-1);
    switch (seg->type) {
    case AS_PATH_SEGMENT_SEQUENCE:
      if (path_segment_add(_path_segment_ref_at(*ppath, length-1), asn))
	return -1;
      break;
    case AS_PATH_SEGMENT_SET:
      seg= path_segment_create(AS_PATH_SEGMENT_SEQUENCE, 1);
      seg->asns[0]= asn;
      path_add_segment(*ppath, seg);
      length++;
      break;
    default:
      abort();
    }
  }
  return length;
}

// ----- path_contains ----------------------------------------------
/**
 * Test if the given AS-Path contains the given AS number.
 *
 * Return value:
 *   1  if the path contains the ASN
 *   0  otherwise
 */
int path_contains(bgp_path_t * path, asn_t asn)
{
  unsigned int i;

  if (path == NULL)
    return 0;

  for (i= 0; i < _path_num_segments(path); i++) {
    if (path_segment_contains(_path_segment_at(path, i), asn) != 0)
      return 1;
  }
  return 0;
}

// ----- _path_at ---------------------------------------------------
/**
 * Return the AS number at the given position whatever the including
 * segment is.
 */
static int _path_at(bgp_path_t * path, int pos, asn_t * asn_ref)
{
  unsigned int index, seg_index;
  bgp_path_seg_t * seg;

  for (index= 0; (pos > 0) && 
	 (index < _path_num_segments(path));
       index++) {
    seg= _path_segment_at(path, index);
    switch (seg->type) {
    case AS_PATH_SEGMENT_SEQUENCE:
      for (seg_index= 0; seg_index ; seg_index++) {
	if (pos == 0) {
	  *asn_ref= seg->asns[seg_index];
	  return 0;
	}
	pos++;
      }
      break;
    case AS_PATH_SEGMENT_SET:
      if (pos == 0) {
	*asn_ref= seg->asns[0];
	return 0;
      }
      pos++;
      break;
    default:
      abort();
    }
  }
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
int path_last_as(bgp_path_t * path, asn_t * asn_ref) {
  bgp_path_seg_t * seg;

  if (path_length(path) <= 0)
    return -1;

  seg= _path_segment_at(path, _path_num_segments(path)-1);
  
  // Check that the segment is of type AS_SEQUENCE 
  assert(seg->type == AS_PATH_SEGMENT_SEQUENCE);
  // Check that segment's size is larger than 0
  assert(seg->length > 0);

  *asn_ref= seg->asns[seg->length-1];
  return 0;
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
int path_first_as(bgp_path_t * path, asn_t * asn_ref) {
  bgp_path_seg_t * seg;

  if (path_length(path) <= 0)
    return -1;

  seg= _path_segment_at(path, 0);

  // Check that the segment is of type AS_SEQUENCE 
  assert(seg->type == AS_PATH_SEGMENT_SEQUENCE);
  // Check that segment's size is larger than 0
  assert(seg->length > 0);

  *asn_ref= seg->asns[0];
  return 0;
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
int path_to_string(bgp_path_t * path, int reverse,
		   char * dst, size_t dst_size)
{
  size_t tInitialSize= dst_size;
  unsigned int index;
  int iWritten= 0;
  unsigned int num_segs;

  if ((path == NULL) || (_path_num_segments(path) == 0)) {
    if (dst_size < 1)
      return -1;
    *dst= '\0';
    return 0;
  }

  if (reverse) {
    num_segs= _path_num_segments(path);
    for (index= num_segs; index > 0; index--) {
      // Write space (if required)
      if (index < num_segs) {
	iWritten= snprintf(dst, dst_size, " ");
	if (iWritten >= dst_size)
	  return -1;
	dst+= iWritten;
	dst_size-= iWritten;
      }

      // Write AS-Path segment
      iWritten= path_segment_to_string(_path_segment_at(path, index-1),
				       reverse, dst, dst_size);
      if (iWritten >= dst_size)
	return -1;
      dst+= iWritten;
      dst_size-= iWritten;
    }

  } else {
    for (index= 0; index < _path_num_segments(path);
	 index++) {
      // Write space (if required)
      if (index > 0) {
	iWritten= snprintf(dst, dst_size, " ");
	if (iWritten >= dst_size)
	  return -1;
	dst+= iWritten;
	dst_size-= iWritten;
      }
      // Write AS-Path segment
      iWritten= path_segment_to_string(_path_segment_at(path, index),
				       reverse, dst, dst_size);
      if (iWritten >= dst_size)
	return -1;
      dst+= iWritten;
      dst_size-= iWritten;
    }
  }
  return tInitialSize-dst_size;
}

// -----[ path_from_string ]-----------------------------------------
/**
 * Convert a string to an AS-Path.
 *
 * Return value:
 *   NULL  if the string does not contain a valid AS-Path
 *   new path otherwise
 */
bgp_path_t * path_from_string(const char * path_str)
{
  unsigned int index;
  const gds_tokens_t * tokens;
  bgp_path_t * path= NULL;
  const char * seg_str;
  bgp_path_seg_t * seg;
  unsigned long asn;

  if (path_tokenizer == NULL)
    path_tokenizer= tokenizer_create(" ", "{[(", "}])");

  if (tokenizer_run(path_tokenizer, path_str) != TOKENIZER_SUCCESS)
    return NULL;

  tokens= tokenizer_get_tokens(path_tokenizer);

  path= path_create();
  for (index= tokens_get_num(tokens); index > 0; index--) {
    seg_str= tokens_get_string_at(tokens, index-1);

    if (!tokens_get_ulong_at(tokens, index-1, &asn)) {
      if (asn > MAX_AS) {
	STREAM_ERR(STREAM_LEVEL_SEVERE, "Error: not a valid AS-Num \"%s\"\n",
		   seg_str);
	path_destroy(&path);
	break;
      }
      path_append(&path, asn);
    } else {
      seg= path_segment_from_string(seg_str);
      if (seg == NULL) {
	STREAM_ERR(STREAM_LEVEL_SEVERE,
		   "Error: not a valid path segment \"%s\"\n", seg_str);
	path_destroy(&path);
	break;
      }
      path_add_segment(path, seg);
    }
  }

  return path;
}

// ----- path_match --------------------------------------------------------
/**
 *
 */
int path_match(bgp_path_t * path, SRegEx * pRegEx)
{
  char acBuffer[1000];
  int iRet = 0;

  if (path_to_string(path, 1, acBuffer, sizeof(acBuffer)) < 0)
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
void path_dump(gds_stream_t * stream, const bgp_path_t * path,
	       int reverse)
{
  int index;

  if ((path == NULL) || (_path_num_segments(path) == 0)) {
    stream_printf(stream, "");
    return;
  }

  if (reverse) {
    for (index= _path_num_segments(path); index > 0; index--) {
      path_segment_dump(stream, _path_segment_at(path, index-1), reverse);
      if (index > 1)
	stream_printf(stream, " ");
    }
  } else {
    for (index= 0; index < _path_num_segments(path); index++) {
      if (index > 0)
	stream_printf(stream, " ");
      path_segment_dump(stream, _path_segment_at(path, index), reverse);
    }
  }
}

// ----- path_hash --------------------------------------------------
/**
 * Universal hash function for string keys (discussed in Sedgewick's
 * "Algorithms in C, 3rd edition") and adapted for AS-Paths.
 */
uint32_t path_hash(const bgp_path_t * path)
{
  uint32_t hash, a = 31415, b = 27183, M = 65521;
  unsigned int i, seg_index;
  bgp_path_seg_t * seg;

  if (path == NULL)
    return 0;

  hash= 0;
  for (i= 0; i < _path_num_segments(path); i++) {
    seg= _path_segment_at(path, i);
    for (seg_index= 0; seg_index < seg->length; seg_index++) {
      hash= (a * hash+seg->asns[seg_index]) % M;
      a= a * b % (M-1);
    }
  }
  return hash;
}

// -----[ path_hash_zebra ]------------------------------------------
/**
 * This is a helper function that computes the hash key of an
 * AS-Path. The function is based on Zebra's AS-Path hashing
 * function.
 */
uint32_t path_hash_zebra(const void * item, unsigned int hash_size)
{
  bgp_path_t * path= (bgp_path_t *) item;
  unsigned int hash= 0;
  uint32_t i, index2;
  bgp_path_seg_t * seg;

  for (i= 0; i < _path_num_segments(path); i++) {
    seg= _path_segment_at(path, i);
    // Note: segment type IDs are equal to those of Zebra
    //(1 AS_SET, 2 AS_SEQUENCE)
    hash+= seg->type;
    for (index2= 0; index2 < seg->length; index2++) {
      hash+= seg->asns[index2] & 255;
      hash+= seg->asns[index2] >> 8;
    }
  }
  return hash;
}

// -----[ path_hash_OAT ]--------------------------------------------
/**
 * Note: 'hash_size' must be a power of 2.
 */
uint32_t path_hash_OAT(const void * item, unsigned int hash_size)
{
  uint32_t hash= 0;
  bgp_path_t * path= (bgp_path_t *) item;
  uint32_t i, index2;
  bgp_path_seg_t * seg;

  for (i= 0; i < _path_num_segments(path); i++) {
    seg= _path_segment_at(path, i);
    // Note: segment type IDs are equal to those of Zebra
    //(1 AS_SET, 2 AS_SEQUENCE)

    hash+= seg->type;
    hash+= (hash << 10);
    hash^= (hash >> 6);
 
    for (index2= 0; index2 < seg->length; index2++) {
      hash+= seg->asns[index2] & 255;
      hash+= (hash << 10);
      hash^= (hash >> 6);
      hash+= seg->asns[index2] >> 8;
      hash+= (hash << 10);
      hash^= (hash >> 6);
    }
  }
  hash+= (hash << 3);
  hash^= (hash >> 11);
  hash+= (hash << 15);
  return hash % hash_size;
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
int path_comparison(bgp_path_t * path1, bgp_path_t * path2)
{
  int length;
  unsigned int i;
  asn_t asn1, asn2;

  if (path_length(path1) < path_length(path2))
    length= path_length(path1);
  else
    length= path_length(path2);

  for (i= 0; i < length; i++) {
    _path_at(path1, i, &asn1);
    _path_at(path2, i, &asn2);
    if (asn1 < asn2)
      return -1;
    else
      if (asn1 > asn2)
        return 1;
  }
  return path_equals(path1, path2);
}

// ----- path_equals ------------------------------------------------
/**
 * Test if two paths are equal. If they are equal, the function
 * returns 1, otherwize, the function returns 0.
 */
int path_equals(const bgp_path_t * path1, const bgp_path_t * path2)
{
  unsigned int i;

  if (path1 == path2)
    return 1;

  if (_path_num_segments(path1) != _path_num_segments(path2))
    return 0;

  for (i= 0; i < _path_num_segments(path1); i++) {
    if (!path_segment_equals(_path_segment_at(path1, i),
			     _path_segment_at(path2, i)))
      return 0;
  }
  return 1;
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
int path_cmp(const bgp_path_t * path1, const bgp_path_t * path2)
{
  unsigned int i;
  int cmp;

  // Null paths are equal
  if (path1 == path2)
    return 0;

  // Null and empty paths are equal
  if (((path1 == NULL) && (_path_num_segments(path2) == 0)) ||
      ((path2 == NULL) && (_path_num_segments(path1) == 0)))
    return 0;

  if (path1 == NULL)
    return -1;
  if (path2 == NULL)
    return 1;
  
  // Longest is first
  if (_path_num_segments(path1) < _path_num_segments(path2))
    return -1;
  else if (_path_num_segments(path1) > _path_num_segments(path2))
    return 1;

  // Equal size paths, inspect individual segments
  for (i= 0; i < _path_num_segments(path1); i++) {
    cmp= path_segment_cmp(_path_segment_at(path1, i),
			   _path_segment_at(path2, i));
    if (cmp != 0)
      return cmp;
  }

  return 0;
}

// -----[ path_str_cmp ]---------------------------------------------
int path_str_cmp(bgp_path_t * path1, bgp_path_t * path2)
{
#define AS_PATH_STR_SIZE 1024
  char acPathStr1[AS_PATH_STR_SIZE];
  char acPathStr2[AS_PATH_STR_SIZE];

  // If paths pointers are equal, no need to compare their content.
  if (path1 == path2)
    return 0;

  assert(path_to_string(path1, 1, acPathStr1, AS_PATH_STR_SIZE) >= 0);
  assert(path_to_string(path2, 1, acPathStr2, AS_PATH_STR_SIZE) >= 0);
   
  return strcmp(acPathStr1, acPathStr2);
}

// -----[ path_remove_private ]--------------------------------------
void path_remove_private(bgp_path_t * path)
{
  unsigned int i;
  bgp_path_seg_t * seg;
  bgp_path_seg_t * new_seg;

  i= 0;
  while (i < _path_num_segments(path)) {
    seg= _path_segment_at(path, i);
    new_seg= path_segment_remove_private(seg);
    if (seg != new_seg) {
      if (new_seg == NULL) {
	ptr_array_remove_at(path, i);
	continue;
      } else {
	_path_segment_set(path, i, new_seg);
      }
    }
    i++;
  }
}

// -----[ _path_destroy ]--------------------------------------------
void _path_destroy()
{
  tokenizer_destroy(&path_tokenizer);
}




