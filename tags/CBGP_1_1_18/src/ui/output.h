// ==================================================================
// @(#)output.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 01/06/2004
// @lastdate 01/06/2004
// ==================================================================

#ifndef __UI_OUTPUT_H__
#define __UI_OUTPUT_H__

#include <stdio.h>

extern int iOptionAutoFlush;

// ----- flushir ----------------------------------------------------
/** Flush if required */
extern void flushir(FILE * pStream);

#endif
