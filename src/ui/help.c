// ==================================================================
// @(#)help.c
//
// Provides functions to obtain help from the CLI in interactive mode.
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 22/11/2002
// @lastdate 25/06/2007
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdlib.h>

#include <libgds/cli_ctx.h>
#include <cli/common.h>

#ifdef HAVE_LIBREADLINE
# ifdef HAVE_READLINE_READLINE_H
#  include <readline/readline.h>
# endif
# include <ui/rl.h>
#endif

// -----[ _cli_help_cmd ]--------------------------------------------
/**
 * Display help for a context: gives the available sub-commands and
 * their parameters.
 */
static void _cli_help_cmd(SCliCmd * pCmd)
{
  int iIndex, iIndex2;
  SCliCmd * pSubCmd;
  SCliParam * pParam;

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
}

// -----[ _cli_help_option ]-----------------------------------------
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

// -----[ _cli_help_param ]------------------------------------------
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

// -----[ cli_help ]-------------------------------------------------
/**
 * This function provides help for the command passed in the given
 * string.
 */
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

#ifdef HAVE_LIBREADLINE
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
#ifdef HAVE_LIBREADLINE
  if (rl_bind_key('?', _rl_help_command) != 0)
    fprintf(stderr, "Error: could not bind.\n");
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
