// ==================================================================
// @(#)help.c
//
// Provides functions to obtain help from the CLI in interactive mode.
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 22/11/2002
// $Id: help.c,v 1.6 2008-04-11 11:02:29 bqu Exp $
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdlib.h>
#include <string.h>

#include <libgds/cli_ctx.h>
#include <libgds/str_util.h>
#include <cli/common.h>
#include <ui/pager.h>

#ifdef HAVE_LIBREADLINE
# ifdef HAVE_READLINE_READLINE_H
#  include <readline/readline.h>
# endif
# include <ui/rl.h>
#endif

// -----[ cli_cmd_get_path ]-----------------------------------------
/**
 * Return the complete command path for the given command.
 */
char * cli_cmd_get_path(SCliCmd * pCmd)
{
  char * pcPath= NULL;

  while (pCmd != NULL) {
    if (strcmp(pCmd->pcName, "")) {
      if (pcPath == NULL)
	pcPath= str_create(pCmd->pcName);
      else {
	str_prepend(&pcPath, "_");
	str_prepend(&pcPath, pCmd->pcName);
      }
    }
    pCmd= pCmd->pParent;
  }
  return pcPath;
}

// -----[ cli_help_cmd_topic ]---------------------------------------
/**
 * Run a pager (typically "less") to display the help file
 * corresponding to the given command.
 *
 *
 */
int cli_help_cmd_topic(SCliCmd * pCmd)
{
  int iResult;
  char * pcFileName= str_create(DOCDIR);
  char * pcPath= cli_cmd_get_path(pCmd);

  if (pcPath == NULL) {
    fprintf(stdout, "\n");
#ifdef HAVE_RL_ON_NEW_LINE
    rl_on_new_line();
#endif
    return 0;
  }

  // Translate the command's path to a valid filename
  str_translate(pcPath, " ", "_");

  fprintf(stdout, "\n");

  str_append(&pcFileName, "/txt/");

  str_append(&pcFileName, pcPath);
  str_destroy(&pcPath);
  str_append(&pcFileName, ".txt");

  iResult= pager_run(pcFileName);

  str_destroy(&pcFileName);

#ifdef HAVE_RL_ON_NEW_LINE
  rl_on_new_line();
#endif

  return iResult;
}

// -----[ _cli_help_cmd ]--------------------------------------------
/**
 * Display help for a context: gives the available sub-commands and
 * their parameters.
 */
#if defined(HAVE_LIBREADLINE) && defined(HAVE_READLINE_READLINE_H)
static void _cli_help_cmd(SCliCmd * pCmd)
{
  /*
  int iIndex, iIndex2;
  SCliCmd * pSubCmd;
  SCliParam * pParam;
  */

  /*if (pCmd->fCommand != NULL) {*/
    cli_help_cmd_topic(pCmd);
    return;
    /*}

  fprintf(stdout, "\n");
  fprintf(stdout, "Syntax:\n");
  cli_cmd_dump(pLogOut, "  ", pCmd);

  if (pCmd->pcHelp != NULL)
    fprintf(stdout, "\nInfo:\n%s\n", pCmd->pcHelp);

  if (cli_cmd_get_num_subcmds(pCmd) > 0) {
    fprintf(stdout, "\nSub-commands:\n");

    for (iIndex= 0; iIndex < cli_cmd_get_num_subcmds(pCmd); iIndex++) {
      pSubCmd= (SCliCmd *) pCmd->pSubCmds->data[iIndex];
      fprintf(stdout, "  %s", pSubCmd->pcName);
      for (iIndex2= 0; iIndex2 < cli_cmd_get_num_params(pSubCmd); iIndex2++) {
	pParam= (SCliParam *) pSubCmd->pParams->data[iIndex2];
	fprintf(stdout, " %s", pParam->pcName);
      }
      fprintf(stdout, "\n");
    }

  }

#ifdef HAVE_RL_ON_NEW_LINE
  rl_on_new_line();
#endif
    */
}
#endif /* HAVE_LIBREADLINE */

// -----[ _cli_help_option ]-----------------------------------------
#if defined(HAVE_LIBREADLINE) && defined(HAVE_READLINE_READLINE_H)
static void _cli_help_option(SCliOption * pOption)
{
  fprintf(stdout, "\n");
  if (pOption->pcInfo == NULL)
    fprintf(stdout, "No help available for option \"--%s\"\n",
	    pOption->pcName);
  else
    fprintf(stdout, "%s\n", pOption->pcInfo);

#ifdef HAVE_RL_ON_NEW_LINE
  rl_on_new_line();
#endif
}
#endif /* HAVE_LIBREADLINE */

// -----[ _cli_help_param ]------------------------------------------
#if defined(HAVE_LIBREADLINE) && defined(HAVE_READLINE_READLINE_H)
static void _cli_help_param(SCliParam * pParam)
{
  fprintf(stdout, "\n");
  if (pParam->pcInfo == NULL)
    fprintf(stdout, "No help available for parameter \"%s\"\n",
	    pParam->pcName);
  else
    fprintf(stdout, "%s\n", pParam->pcInfo);

#ifdef HAVE_RL_ON_NEW_LINE
  rl_on_new_line();
#endif
}
#endif /* HAVE_LIBREADLINE */

// -----[ cli_help ]-------------------------------------------------
/**
 * This function provides help for the command passed in the given
 * string.
 */
#if defined(HAVE_LIBREADLINE) && defined(HAVE_READLINE_READLINE_H)
void cli_help(SCli * pCli, char * pcLine, int iPos)
{
  SCliCmd * pCtxCmd= NULL;
  int iParamIndex;
  char * pcStartCmd;
  char * pcPos;
  int iStatus;
  void * pCtx;

  // Get the current command context.
  pCtxCmd= cli_get_cmd_context(pCli);

  // Retrieve the content of the readline buffer, from the first
  // character to the first occurence of '?'. On this basis, try to
  // match the command for which help was requested.
  pcPos= ((char*) pcLine)+iPos;//strchr(pcLine, '?');
  if ((int) (pcPos-pcLine) > 0) {

    // This is the command line prefix
    pcStartCmd= (char *) malloc((((int) (pcPos-pcLine))+1)*sizeof(char));
    strncpy(pcStartCmd, rl_line_buffer, (int) (pcPos-pcLine));
    pcStartCmd[(int) (pcPos-pcLine)]= '\0';

    // Try to match it with the CLI command tree. On success, retrieve
    // the current command and parameter index. THIS CURRENTLY DOES NOT
    // WORK WITH OPTIONS !!!
    iStatus= cli_cmd_match(pCli, pCtxCmd, pcStartCmd, NULL, &pCtx);

    free(pcStartCmd);
  } else {
    iStatus= CLI_MATCH_COMMAND;
    pCtx= pCtxCmd;
    iParamIndex= 0;
  }

  switch (iStatus) {
  case CLI_MATCH_NOTHING:
    fprintf(stdout, "\n");
    fprintf(stdout, "Sorry, no help is available on this topic\n");
    break;
  case CLI_MATCH_COMMAND:
    _cli_help_cmd((SCliCmd *) pCtx);
    break;
  case CLI_MATCH_OPTION_VALUE:
    _cli_help_option((SCliOption *) pCtx);
    break;
  case CLI_MATCH_PARAM_VALUE:
    _cli_help_param((SCliParam *) pCtx);
    break;
  default: abort();
  }

#ifdef HAVE_RL_ON_NEW_LINE
    rl_on_new_line();
#endif
}
#endif /* HAVE_LIBREADLINE */

#if defined(HAVE_LIBREADLINE) && defined(HAVE_READLINE_READLINE_H)
static int _rl_help_command(int count, int key)
{
  SCli * pCli= cli_get();
  cli_help(pCli, rl_line_buffer, rl_point);
  return 0;
}
#endif /* HAVE_LIBREADLINE */


/////////////////////////////////////////////////////////////////////
// INITIALIZATION AND FINALIZATION SECTION
/////////////////////////////////////////////////////////////////////

// -----[ _rl_help_init ]--------------------------------------------
void _rl_help_init()
{
#if defined(HAVE_LIBREADLINE) && defined(HAVE_READLINE_READLINE_H)
  if (rl_bind_key('?', _rl_help_command) != 0)
    fprintf(stderr, "Error: could not bind '?' key to help.\n");
  else
    fprintf(stderr, "help is bound to '?' key\n");

#ifdef HAVE_RL_ON_NEW_LINE
  rl_on_new_line();
#endif /* HAVE_RL_ON_NEW_LINE */

#endif /* HAVE_LIBREADLINE */
}

// -----[ _rl_help_done ]--------------------------------------------
void _rl_help_done()
{
}
