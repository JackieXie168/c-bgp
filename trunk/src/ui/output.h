// ==================================================================
// @(#)output.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 01/06/2004
// $Id: output.h,v 1.4 2009-03-24 16:29:41 bqu Exp $
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
