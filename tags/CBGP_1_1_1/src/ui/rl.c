// ==================================================================
// @(#)rl.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 23/02/2004
// @lastdate 23/02/2004
// ==================================================================
// This code is explained in the GNU readline manual.

#include <stdio.h>
#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>

/* A static variable for holding the line. */
static char * pcLineRead= NULL;

// ---- rl_gets -----------------------------------------------------
/**
 * Read a string, and return a pointer to it.
 * Returns NULL on EOF.
 */
char * rl_gets(char * pcPrompt)
{
  /* If the buffer has already been allocated,
     return the memory to the free pool. */
  if (pcLineRead) {
    free(pcLineRead);
    pcLineRead= (char *) NULL;
  }

  /* Get a line from the user. */
  pcLineRead= readline(pcPrompt);

  /* If the line has any text in it,
     save it on the history. */
  if (pcLineRead && *pcLineRead)
    add_history(pcLineRead);

  return (pcLineRead);
}

/////////////////////////////////////////////////////////////////////
// INITIALIZATION AND FINALIZATION SECTION
/////////////////////////////////////////////////////////////////////

void _rl_destroy() __attribute__((destructor));

// ----- _rl_destroy ------------------------------------------------
/**
 *
 */
void _rl_destroy()
{
  if (pcLineRead != NULL) {
    free(pcLineRead);
    pcLineRead= NULL;
  }
}
