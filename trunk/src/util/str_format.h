// ==================================================================
// @(#)str_format.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 22/02/2008
// $Id: str_format.h,v 1.2 2009-03-24 16:30:03 bqu Exp $
// ==================================================================

#ifndef __UTIL_STR_FORMAT_H__
#define __UTIL_STR_FORMAT_H__

#include <libgds/stream.h>

typedef int (*FFormatForEach)(gds_stream_t * stream, void * ctx,
			       char format);

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ str_format_for_each ]------------------------------------
  int str_format_for_each(gds_stream_t * stream, FFormatForEach fFunction,
			  void * ctx, const char * format);

#ifdef __cplusplus
}
#endif

#endif /* __UTIL_STR_FORMAT_H__ */
