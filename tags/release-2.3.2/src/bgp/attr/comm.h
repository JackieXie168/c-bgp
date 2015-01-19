// ==================================================================
// @(#)comm.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 23/05/2003
// $Id: comm.h,v 1.1 2009-03-24 13:41:26 bqu Exp $
// ==================================================================

/**
 * \file
 * Provide functions to manage BGP Communities attributes.
 */

#ifndef __BGP_COMM_H__
#define __BGP_COMM_H__

#include <libgds/stream.h>

#include <bgp/attr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ comms_create ]-------------------------------------------
  /**
   * Create a Communities attribute.
   */
  bgp_comms_t * comms_create();

  // -----[ comms_destroy ]------------------------------------------
  /**
   * Destroy a Communities attribute.
   */
  void comms_destroy(bgp_comms_t ** comms_ref);

  // -----[ comms_dup ]----------------------------------------------
  /**
   * Duplicate a Communities attribute.
   */
  bgp_comms_t * comms_dup(const bgp_comms_t * comms);

  // -----[ comms_length ]-------------------------------------------
  /**
   * Return the number of values in a Communities attribute.
   */
  static inline unsigned int comms_length(const bgp_comms_t * comms) {
    if (comms == NULL)
      return 0;
    return comms->num;
  }

  // -----[ comms_add ]----------------------------------------------
  /**
   * Add a value to a Communities attribute.
   */
  int comms_add(bgp_comms_t ** comms_ref, bgp_comm_t comm);

  // -----[ comms_remove ]-------------------------------------------
  /**
   * Remove a value from a Communities attribute.
   */
  void comms_remove(bgp_comms_t ** comms_ref, bgp_comm_t comm);

  // -----[ comms_contains ]-----------------------------------------
  /**
   * Test if a value is contained in a Communities attribute.
   */
  int comms_contains(bgp_comms_t * comms, bgp_comm_t comm);

  // -----[ comms_equals ]-------------------------------------------
  /**
   * Test if two Communities attributes are equal.
   */
  int comms_equals(const bgp_comms_t * comms1,
		   const bgp_comms_t * comms2);

  // -----[ comm_value_from_string ]---------------------------------
  /**
   *
   */
  int comm_value_from_string(const char * comm_str, bgp_comm_t * comm);

  // -----[ comm_value_dump ]----------------------------------------
  void comm_value_dump(gds_stream_t * stream, bgp_comm_t comm,
		       int as_text);

  // ----- comm_dump ------------------------------------------------
  void comm_dump(gds_stream_t * stream, const bgp_comms_t * comms,
		 int as_text);

  // -----[ comm_to_string ]-----------------------------------------
  int comm_to_string(bgp_comms_t * comms, char * buf, size_t buf_size);

  // -----[ comm_from_string ]---------------------------------------
  bgp_comms_t * comm_from_string(const char * comms_str);

  // -----[ comms_cmp ]----------------------------------------------
  /**
   * Compare two Communities attributes.
   */
  int comms_cmp(const bgp_comms_t * comms1, const bgp_comms_t * comms2);

  // -----[ _comm_destroy ]------------------------------------------
  /**
   * \internal
   * This function must be called by the CBGP library finalization
   * function.
   */
  void _comm_destroy();

#ifdef __cplusplus
}
#endif

#endif /* __BGP_COMM_H__ */
