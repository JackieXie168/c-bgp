// ==================================================================
// @(#)as-level-util.h
//
// Provides utility functions to manage and handle large AS-level
// topologies.
//
// @author Bruno Quoitin (bruno.quoitin@umons.as.be)
// @date 21/06/2007
// $Id: util.h,v 1.1 2009-03-24 13:39:08 bqu Exp $
// ==================================================================

#ifndef __BGP_ASLEVEL_UTIL_H__
#define __BGP_ASLEVEL_UTIL_H__

#include <bgp/as.h>

#ifndef AS_SIZE_32
#define AS_SET_SIZE MAX_AS/(sizeof(unsigned int)*8)
#define AS_SET_DEFINE(SET) unsigned int SET[AS_SET_SIZE]
#define AS_SET_INIT(SET) memset(SET, 0, sizeof(SET));
#define IS_IN_AS_SET(SET,ASN) ((SET[ASN / (sizeof(unsigned int)*8)] &\
				(1 << (ASN % (sizeof(unsigned int)*8)))) != 0)
#define AS_SET_PUT(SET,ASN) SET[ASN / (sizeof(unsigned int)*8)]|=\
  1 << (ASN % (sizeof(unsigned int)*8));
#define AS_SET_CLEAR(SET,ASN) SET[ASN / (sizeof(unsigned int)*8)]&= \
  ~(1 << (ASN % (sizeof(unsigned int)*8)));
#endif /* AS_SIZE_32 */

#endif /* __BGP_ASLEVEL_UTIL_H__ */
