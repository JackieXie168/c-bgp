// ==================================================================
// @(#)str_format.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 22/02/2008
// @lastdate 22/02/2008
// ==================================================================

#ifndef __UTIL_STR_FORMAT_H__
#define __UTIL_STR_FORMAT_H__

#include <libgds/log.h>

typedef int (*FFormatForEach)(SLogStream * pStream, void * pContext,
			       char cFormat);

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ str_format_for_each ]------------------------------------
  int str_format_for_each(SLogStream * pStream, FFormatForEach fFunction,
			  void * pContext, const char * pcFormat);

#ifdef __cplusplus
}
#endif

#endif /* __UTIL_STR_FORMAT_H__ */
