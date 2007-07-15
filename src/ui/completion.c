// ==================================================================
// @(#)completion.c
//
// Provides functions to auto-complete commands/parameters in the CLI
// in interactive mode.
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 22/11/2002
// @lastdate 25/06/2007
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdlib.h>
#include <string.h>

#include <libgds/cli.h>
#include <libgds/cli_ctx.h>
#include <libgds/memory.h>

#include <cli/common.h>

#ifdef HAVE_LIBREADLINE
#ifdef HAVE_READLINE_READLINE_H
#include <readline/readline.h>

// -----[ completion state ]-----
/* NOTE: this will not work in a multi-threaded environment. */
#ifdef HAVE_RL_COMPLETION_MATCHES
static SCliCmd * pComplCmd= NULL;       // Current command
static SCliCmd * pComplCtxCmd= NULL;    // Context command
static SCliParam * pComplParam= NULL;   // Current parameter
static SCliOption * pComplOption= NULL; // Current option
#endif /* HAVE_RL_COMPLETION_MATCHES */

// -----[ _cli_display_completion ]----------------------------------
/**
 * This function displays the available completions in a non-default
 * way. In C-BGP, we show a vertical list of commands while in the
 * default readline behaviour, the matches are displayed on an
 * horizontal line.
 *
 * NOTE: the first item in 'matches' corresponds to the value being
 * completed, while the subsequent items are the available
 * matches. This is not explicitly explained in readline's
 * documentation.
 */
static void _cli_display_completion(char **matches, int num_matches,
				    int max_length)
{
  int iIndex;

  fprintf(stdout, "\n");
  for (iIndex= 1; iIndex <= num_matches; iIndex++) {
    fprintf(stdout, "\t%s\n", matches[iIndex]);
  }
#ifdef HAVE_RL_ON_NEW_LINE
  rl_on_new_line();
#endif
}

// -----[ _cli_compl_cmd_generator ]---------------------------------
/**
 * This function generates completion for CLI commands. It relies on
 * the list of sub-commands of the current command.
 *
 * Note: use malloc() instead of MALLOC() since GNU readline is
 * responsible for freeing the memory. Here strdup() is used.
 */
#ifdef HAVE_RL_COMPLETION_MATCHES
static char * _cli_compl_cmd_generator(const char * pcText, int iState)
{
  static int iIndex;
  SCliCmd * pCmd;

  // Check that a command is to be completed.
  if (pComplCmd == NULL)
    return NULL;

  // First call, initialize generator's state.
  if (iState == 0)
    iIndex= 0;

  // Obtain matching commands. List all commands, check if they match the
  // given prefix.
  while (iIndex < ptr_array_length(pComplCmd->pSubCmds)) {
    pCmd= (SCliCmd *) pComplCmd->pSubCmds->data[iIndex];
    iIndex++;
    if (!strncmp(pcText, pCmd->pcName, strlen(pcText))) {
      rl_attempted_completion_over= 0;
      return strdup(pCmd->pcName);
    }
  }

  rl_attempted_completion_over= 1;
  return NULL;
}
#endif /* HAVE_RL_COMPLETION_MATCHES */

// -----[ _cli_compl_options_generator ]-----------------------------
/**
 * Generates the list of options of the current command.
 *
 * Note: use malloc() instead of MALLOC() since GNU readline is
 * responsible for freeing the memory.
 */
#ifdef HAVE_RL_COMPLETION_MATCHES
static char * _cli_compl_options_generator(const char * pcText, int iState)
{
  static int iIndex;
  SCliOption * pOption;
  char * pcOptionName;

  // Check that a command is to be completed.
  if (pComplCmd == NULL)
    return NULL;

  // Skip option name's prefix ("--") 
  pcText+= 2;

  // First call, initialize generator's state.
  if (iState == 0)
    iIndex= 0;

  // Obtain matching options. List all available options, check if
  // they match the searched prefix, return "--" + option name.
  while (iIndex < ptr_array_length(pComplCmd->pOptions)) {
    pOption= (SCliOption *) pComplCmd->pOptions->data[iIndex];
    iIndex++;
    if (!strncmp(pcText, pOption->pcName, strlen(pcText))) {
      pcOptionName= (char *) malloc((strlen(pOption->pcName)+3)*sizeof(char));
      strcpy(pcOptionName, "--");
      strcpy(pcOptionName+2, pOption->pcName);
      rl_attempted_completion_over= 0;
      return pcOptionName;
    }
  }

  rl_attempted_completion_over= 1;
  return NULL;
}
#endif /* HAVE_RL_COMPLETION_MATCHES */

// -----[ _cli_compl_option_generator ]------------------------------
/**
 *
 */
#ifdef HAVE_RL_COMPLETION_MATCHES
static char * _cli_compl_option_generator(const char * pcText, int iState)
{
  static int iIndex= 0;
  //char * pcMatch;

  // Check that a valid option is to be completed.
  if (pComplOption == NULL)
    return NULL;

  // First call, initialize generator's state
  if (iState == 0)
    iIndex= 0;

  // If the option supports a completion function (not supported yet).
  /*if (pComplOption->fEnum != NULL) {

    if ((pcMatch= pComplParam->fEnum(pcText, iIndex)) == NULL)
      rl_attempted_completion_over= 1;
    iIndex++;
    return pcMatch;

    }*/

  rl_attempted_completion_over= 1;
  return NULL;
}
#endif /* HAVE_RL_COMPLETION_MATCHES */

// -----[ _cli_compl_param_generator ]-------------------------------
/**
 * This function generates completion for CLI parameter values. The
 * generator first relies on a possible enumeration function [not
 * implemented yet], then on a parameter checking function.
 *
 * Note: use malloc() for the returned matches, not MALLOC().
 */
#ifdef HAVE_RL_COMPLETION_MATCHES
static char * _cli_compl_param_generator(const char * pcText, int iState)
{
  static int iIndex;
  char * pcMatch= NULL;

  // Check that a valid parameter is to be completed.
  if (pComplParam == NULL)
    return NULL;

  // If the parameter supports a completion function.
  if (pComplParam->fEnum != NULL) {

    if (iState == 0) {
      iIndex= 0;
    }
    pcMatch= pComplParam->fEnum(pcText, iIndex);
    if (pcMatch == NULL)
      rl_attempted_completion_over= 1;
    iIndex++;

  } else {

    if (iState == 0) {
      if ((pComplParam->fCheck != NULL) &&
	  (pComplParam->fCheck(pcText))) {

	// If there is a parameter checker and the parameter value is
	// accepted, perform the completion.
	pcMatch= (char*) malloc((strlen(pcText)+1)*sizeof(char));
	strcpy(pcMatch, pcText);
	rl_attempted_completion_over= 1;

      } else {

	// Otherwize, prevent the completion.
	rl_attempted_completion_over= 1;

      }
    }     
  }
    
  return pcMatch;
}
#endif /* HAVE_RL_COMPLETION_MATCHES */

// -----[ _cli_compl ]-----------------------------------------------
/**
 *
 */
#ifdef HAVE_RL_COMPLETION_MATCHES
static char ** _cli_compl (const char * pcText, int iStart, int iEnd)
{
  SCli * pCli= cli_get();
  char * pcStartCmd;
  char ** apcMatches= NULL;
  int iStatus= 0;
  void * pContext= NULL;

  // Authorize readline's completion.
  rl_attempted_completion_over= 0;

  // Find context command.
  pComplCtxCmd= cli_get_cmd_context(pCli);
  
  // Match beginning of readline buffer.
  if (iStart == 0) {
    pContext= pComplCtxCmd;
    iStatus= CLI_MATCH_COMMAND;

  } else {
    pcStartCmd= (char *) MALLOC((iStart+1)*sizeof(char));
    strncpy(pcStartCmd, rl_line_buffer, iStart);
    pcStartCmd[iStart]= '\0';

    iStatus= cli_cmd_match(pCli, pComplCtxCmd, pcStartCmd,
			   (char *) pcText, &pContext);
    FREE(pcStartCmd);
  }

  switch (iStatus) {
  case CLI_MATCH_NOTHING:
    rl_attempted_completion_over= 1;
    apcMatches= NULL;
    break;
  case CLI_MATCH_COMMAND:
    pComplCmd= (SCliCmd *) pContext;
    apcMatches= rl_completion_matches(pcText, _cli_compl_cmd_generator);
    break;
  case CLI_MATCH_OPTION_NAMES:
    pComplCmd= (SCliCmd *) pContext;
    apcMatches= rl_completion_matches(pcText, _cli_compl_options_generator);
    break;
  case CLI_MATCH_OPTION_VALUE:
    pComplOption= (SCliOption *) pContext;
    apcMatches= rl_completion_matches(pcText, _cli_compl_option_generator);
    break;
  case CLI_MATCH_PARAM_VALUE:
    pComplParam= (SCliParam *) pContext;
    apcMatches= rl_completion_matches(pcText, _cli_compl_param_generator);
    break;
  default: abort();
  }

  return apcMatches;
}
#endif /* HAVE_RL_COMPLETION_MATCHES */

#endif /* HAVE_READLINE_READLINE_H */
#endif /* HAVE_LIBREADLINE */


/////////////////////////////////////////////////////////////////////
// INITIALIZATION AND FINALIZATION SECTION
/////////////////////////////////////////////////////////////////////

// -----[ _rl_completion_init ]--------------------------------------
/**
 * Initializes the auto-completion, in cooperation with GNU readline.
 * -> replaces the completion display function with ours.
 * -> replaces the completion function with ours if readline's
 *    rl_completion_matches() is available.
 */
void _rl_completion_init()
{
#ifdef HAVE_LIBREADLINE
#ifdef HAVE_READLINE_READLINE_H

#ifdef HAVE_RL_COMPLETION_MATCHES
  // Setup alternate completion function.
  rl_attempted_completion_function= _cli_compl;
#endif /* HAVE_RL_COMPLETION_MATCHES */

  // Setup alternate completion display function.
  rl_completion_display_matches_hook= _cli_display_completion;

#endif /* HAVE_READLINE_READLINE_H */
#endif /* HAVE_LIBREADLINE */
}

// -----[ _rl_completion_done ]--------------------------------------
/**
 * Frees the resources allocated for the auto-completion.
 */
void _rl_completion_done()
{
}
