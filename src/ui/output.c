// ==================================================================
// @(#)output.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 01/06/2004
// $Id: output.c,v 1.3 2009-03-24 16:29:41 bqu Exp $
// ==================================================================

#include <stdio.h>
#include <stdlib.h>
#include <ui/output.h>

int iOptionAutoFlush= 0;
int iOptionDebug= 0;
int iOptionExitOnError= 1;

// ----- flushir ----------------------------------------------------
/** Flush if required */
inline void flushir(FILE * stream)
{
  if (iOptionAutoFlush) {
    if (fflush(stream)) {
      perror("fflush: ");
      exit(EXIT_FAILURE);
    }
  }
  
}



