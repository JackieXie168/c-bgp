// ==================================================================
// @(#)regex.h
//
// @author Sebastien Tandel (standel@info.ucl.ac.be)
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 29/09/2004
// @lastdate 27/01/2005
// ==================================================================

#ifndef __UTIL_REGEX_H__
#define __UTIL_REGEX_H__

#include <libgds/types.h>

typedef struct RegEx SRegEx;

// ----- regex_int ---------------------------------------------------
SRegEx * regex_init(char * pattern, const unsigned int uMaxResult);
// ----- regex_search ------------------------------------------------
int regex_search(SRegEx * pRegEx, const char * sExp);
// ----- regex_get_result --------------------------------------------
char * regex_get_result(SRegEx * pRegEx, const char * sExp);
// ----- regex_finalize ----------------------------------------------
void regex_finalize(SRegEx ** pRegEx);

#endif /* __UTIL_REGEX_H__ */
