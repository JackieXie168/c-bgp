// ==================================================================
// @(#)output.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 01/06/2004
// @lastdate 02/06/2004
// ==================================================================

#include <stdio.h>
#include <stdlib.h>
#include <ui/output.h>

int iOptionAutoFlush= 0;
int iOptionDebug= 0;

// ----- flushir ----------------------------------------------------
/** Flush if required */
inline void flushir(FILE * pStream)
{
  if (iOptionAutoFlush) {
    if (fflush(pStream)) {
      perror("fflush: ");
      exit(EXIT_FAILURE);
    }
  }
  
}



