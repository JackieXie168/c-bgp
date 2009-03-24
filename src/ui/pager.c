// ==================================================================
// @(#)pager.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 30/11/2007
// $Id: pager.c,v 1.2 2009-03-24 16:29:41 bqu Exp $
// ==================================================================

#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <ui/pager.h>

// -----[ pager_run ]------------------------------------------------
int pager_run(const char * filename) {
  char * const args[]= {
    (char *) PAGER_CMD,
    (char *) filename,
    NULL
  };
  int result;
  int status;
  struct stat sb;

  // Check that file exists
  if (stat(filename, &sb) < 0)
    return -1;

  result= fork();
  if (result < 0)
    return -1;

  // Parent process (wait until child is terminated)
  if (result > 0) {
    waitpid(result, &status, 0);
    return PAGER_SUCCESS;
  }

  // Child process (pager)
  result= execvp(PAGER_CMD, args);
  if (result < 0)
    exit(EXIT_FAILURE);
  exit(EXIT_SUCCESS);
}
