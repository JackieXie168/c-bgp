// ==================================================================
// @(#)output.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 01/06/2004
// @lastdate 18/03/2005
// ==================================================================

#ifndef __UI_OUTPUT_H__
#define __UI_OUTPUT_H__

#include <stdio.h>

extern int iOptionAutoFlush;
extern int iOptionDebug;

// ----- flushir ----------------------------------------------------
/** Flush if required */
extern void flushir(FILE * pStream);

#endif
