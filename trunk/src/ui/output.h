// ==================================================================
// @(#)output.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 01/06/2004
// @lastdate 16/01/2007
// ==================================================================

#ifndef __UI_OUTPUT_H__
#define __UI_OUTPUT_H__

#include <stdio.h>

extern int iOptionAutoFlush;
extern int iOptionDebug;
extern int iOptionExitOnError;

#ifdef __cplusplus
extern "C" {
#endif

  // ----- flushir --------------------------------------------------
  /** Flush if required */
  void flushir(FILE * pStream);

#ifdef __cplusplus
}
#endif

#endif /* __UI_OUTPUT_H__ */
