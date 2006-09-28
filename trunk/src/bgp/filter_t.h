// ==================================================================
// @(#)filter_t.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 27/11/2002
// @lastdate 28/09/2006
// ==================================================================

#ifndef __FILTER_T_H__
#define __FILTER_T_H__

#include <libgds/sequence.h>

#define FILTER_OUT 0
#define FILTER_IN  1

typedef struct {
  unsigned char uCode;
  unsigned char uSize;
  unsigned char auParams[0];
} SFilterMatcher;

typedef struct TFilterAction {
  unsigned char uCode;
  unsigned char uSize;
  struct TFilterAction * pNextAction;
  unsigned char auParams[0];
} SFilterAction;

typedef struct {
  SFilterMatcher * pMatcher;
  SFilterAction * pAction;
} SFilterRule;

typedef struct {
  SSequence * pSeqRules;
} SFilter;

#endif
