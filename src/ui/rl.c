// ==================================================================
// @(#)rl.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 23/02/2004
// @lastdate 31/03/2006
// ==================================================================
// Help about this code can be found in the GNU readline manual.

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <libgds/types.h>

#include <stdio.h>
#include <stdlib.h>
#include <ui/rl.h>

#ifdef HAVE_READLINE_READLINE_H
#include <readline/readline.h>
#include <readline/history.h>
#endif

#define CBGP_HISTFILE ".cbgp_history"

/* A static variable for holding the line. */
static char * pcLineRead= NULL;

/* Maximum number of lines to hold in readline's history */
int iHistFileSize= 500;

// ---- rl_gets -----------------------------------------------------
/**
 * Read a string, and return a pointer to it.
 * Returns NULL on EOF.
 */
char * rl_gets(char * pcPrompt)
{
#define MAX_LINE_READ 1024

#ifdef HAVE_LIBREADLINE
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
  if (pcLineRead && *pcLineRead) {
    add_history(pcLineRead);
    stifle_history((int) iHistFileSize);
  }
#else
  /* If the buffer has not yet been allocated, allocate it.
     The buffer will be freed by the destructor function. */
  if (pcLineRead == NULL)
    pcLineRead= (char *) malloc(MAX_LINE_READ*sizeof(char));

  /* Get at most MAX_LINE_READ-1 characters from stdin */
  fgets(pcLineRead, MAX_LINE_READ, stdin);
#endif

  return (pcLineRead);
}

/////////////////////////////////////////////////////////////////////
// INITIALIZATION AND FINALIZATION SECTION
/////////////////////////////////////////////////////////////////////

// ----- _rl_init ---------------------------------------------------
/**
 *
 */
void _rl_init()
{
#ifdef HAVE_LIBREADLINE
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
#endif
}

// ----- _rl_destroy ------------------------------------------------
/**
 *
 */
void _rl_destroy()
{
#ifdef HAVE_LIBREADLINE
  char * pcEnvHistFile;
  char * pcEnvHome;

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
