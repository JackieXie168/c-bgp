// ==================================================================
// @(#)pager.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 30/11/2007
// @lastdate 04/12/2007
// ==================================================================

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <ui/pager.h>

// -----[ pager_run ]------------------------------------------------
int pager_run(char * pcFileName) {
  int iResult;
  int iStatus;
  char * pcArgs[]= {
    PAGER_CMD,
    pcFileName,
    NULL
  };

  iResult= fork();
  if (iResult < 0)
    return -1;

  // Parent process (wait until child is terminated)
  if (iResult > 0) {
    waitpid(iResult, &iStatus, 0);
    return PAGER_SUCCESS;
  }

  // Child process (pager)
  iResult= execvp(PAGER_CMD, pcArgs);
  if (iResult < 0)
    exit(EXIT_FAILURE);
  exit(EXIT_SUCCESS);
}
