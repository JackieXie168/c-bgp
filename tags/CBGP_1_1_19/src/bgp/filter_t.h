// ==================================================================
// @(#)filter_t.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 27/11/2002
// @lastdate 16/11/2004
// ==================================================================

#ifndef __FILTER_T_H__
#define __FILTER_T_H__

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
