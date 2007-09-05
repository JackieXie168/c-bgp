// ==================================================================
// @(#)rl.c
//
// Provides an interactive CLI, based on GNU readline. If GNU
// readline isn't available, provides a basic replacement CLI.
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 23/02/2004
// @lastdate 28/08/2007
// ==================================================================

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <libgds/types.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libgds/cli.h>
#include <libgds/cli_ctx.h>

#include <cli/common.h>
#include <ui/rl.h>
#include <ui/completion.h>
#include <ui/help.h>

#ifdef HAVE_READLINE_READLINE_H
#include <readline/readline.h>
#endif
#ifdef HAVE_READLINE_HISTORY_H
#include <readline/history.h>
#endif

#define CBGP_HISTFILE ".cbgp_history"
#define MAX_LINE_READ 1024

// A static variable for holding the line.
static char * pcLineRead= NULL;

// Maximum number of lines to hold in readline's history.
#ifdef HAVE_LIBREADLINE
static int iHistFileSize= 500;
#endif /* HAVE_LIBREADLINE */


// ---- rl_gets -----------------------------------------------------
/**
 * Reads a string, and return a pointer to it. This function is
 * inspired from an example in the GNU readline manual.
 *
 * Returns NULL on EOF.
 */
char * rl_gets()
{
#if defined(HAVE_LIBREADLINE) && defined(HAVE_READLINE_READLINE_H)

  // If the buffer has already been allocated,
  // return the memory to the free pool.
  if (pcLineRead) {
    free(pcLineRead);
    pcLineRead= (char *) NULL;
  }

  // Show the prompt and get a line from the user.
  pcLineRead= readline(cli_context_to_string(cli_get()->pCtx, "cbgp"));

  // If the line has any text in it, save it on the history.
  if (pcLineRead && *pcLineRead) {
    add_history(pcLineRead);
    stifle_history((int) iHistFileSize);
  }

#else

  // If the buffer has not yet been allocated, allocate it.
  // The buffer will be freed by the destructor function.
  if (pcLineRead == NULL)
    pcLineRead= (char *) malloc(MAX_LINE_READ*sizeof(char));

  // Print the prompt
  fprintf(stdout, cli_context_to_string(cli_get()->pCtx, "cbgp"));

  // Get at most MAX_LINE_READ-1 characters from stdin
  fgets(pcLineRead, MAX_LINE_READ, stdin);

#endif

  return (pcLineRead);
}


/////////////////////////////////////////////////////////////////////
// INITIALIZATION AND FINALIZATION SECTION
/////////////////////////////////////////////////////////////////////

// ----- _rl_init ---------------------------------------------------
/**
 * Initializes the interactive CLI, using GNU readline. This function
 * loads the CLI history if the environment variable CBGP_HISTFILE is
 * defined.
 */
void _rl_init()
{
#if defined(HAVE_LIBREADLINE) && defined(HAVE_READLINE_HISTORY_H)
  char * pcEnvHistFile;
  char * pcEnvHistFileSize;
  char * pcEnvHome;
  char * endptr;
  long int lTmp;

  pcEnvHistFile= getenv("CBGP_HISTFILE");
  if (pcEnvHistFile != NULL) {
    if (strlen(pcEnvHistFile) > 0) {
      read_history(pcEnvHistFile);
    } else {
      pcEnvHome= getenv("HOME");
      if (pcEnvHome != NULL) { /* use ~/.cbgp_history file */
	pcEnvHistFile= (char *) malloc(sizeof(char)*(2+strlen(CBGP_HISTFILE)+
						     strlen(pcEnvHome)));
	sprintf(pcEnvHistFile, "%s/%s", pcEnvHome, CBGP_HISTFILE);
	read_history(pcEnvHistFile);
	free(pcEnvHistFile);
      } else {
	read_history(NULL); /* use default ~/.history file */
      }
    }
  }

  pcEnvHistFileSize= getenv("CBGP_HISTFILESIZE");
  if (pcEnvHistFileSize != NULL) {
    lTmp= strtol(pcEnvHistFileSize, &endptr, 0);
    if ((*endptr == '\0') && (lTmp < INT_MAX))
      iHistFileSize= (int) lTmp;
  }

  _rl_completion_init();
  _rl_help_init();

#endif
}

// ----- _rl_destroy ------------------------------------------------
/**
 * Frees the resources used by the interactive CLI. This function
 * also stores the CLI history in a file if the environment variable
 * CBGP_HISTFILE is defined.
 */
void _rl_destroy()
{
#if defined(HAVE_LIBREADLINE) && defined(HAVE_READLINE_HISTORY_H)
  char * pcEnvHistFile;
  char * pcEnvHome;

  _rl_completion_done();
  _rl_help_done();

  pcEnvHistFile= getenv("CBGP_HISTFILE");
  if (pcEnvHistFile != NULL) {
    if (strlen(pcEnvHistFile) > 0) {
      write_history(pcEnvHistFile);
    } else {
      pcEnvHome= getenv("HOME");
      if (pcEnvHome != NULL) { /* use ~/.cbgp_history file */
	pcEnvHistFile= (char *) malloc(sizeof(char)*(2+strlen(CBGP_HISTFILE)+
						     strlen(pcEnvHome)));
	sprintf(pcEnvHistFile, "%s/%s", pcEnvHome, CBGP_HISTFILE);
	write_history(pcEnvHistFile);
	free(pcEnvHistFile);
      } else {
	write_history(NULL); /* use default ~/.history file */
      }
    }
  }
#endif

  if (pcLineRead != NULL) {
    free(pcLineRead);
    pcLineRead= NULL;
  }
}
