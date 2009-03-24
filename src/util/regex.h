// ==================================================================
// @(#)regex.h
//
// @author Sebastien Tandel (standel@info.ucl.ac.be)
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 29/09/2004
// $Id: regex.h,v 1.4 2009-03-24 16:30:03 bqu Exp $
// ==================================================================

#ifndef __UTIL_REGEX_H__
#define __UTIL_REGEX_H__

#include <libgds/types.h>

typedef struct RegEx SRegEx;

#ifdef __cplusplus
extern "C" {
#endif

  // ----- regex_int ------------------------------------------------
  SRegEx * regex_init(const char * pattern, const unsigned int uMaxResult);
  // ----- regex_reinit ---------------------------------------------
  void regex_reinit(SRegEx * pRegEx);
  // ----- regex_search ---------------------------------------------
  int regex_search(SRegEx * pRegEx, const char * sExp);
  // ----- regex_get_result -----------------------------------------
  char * regex_get_result(SRegEx * pRegEx, const char * sExp);
  // ----- regex_finalize -------------------------------------------
  void regex_finalize(SRegEx ** pRegEx);

#ifdef __cplusplus
}
#endif

#endif /* __UTIL_REGEX_H__ */
