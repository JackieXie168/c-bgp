// ==================================================================
// @(#)main.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @author Sebastien Tandel (standel@info.ucl.ac.be)
// @date 22/11/2002
// @lastdate 31/01/2005
// ==================================================================

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <libgds/cli_ctx.h>
#include <limits.h>
#include <libgds/log.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <bgp/as.h>
#include <bgp/peer_t.h>
#include <bgp/predicate_parser.h>
#include <bgp/qos.h>
#include <bgp/rexford.h>
#include <cli/common.h>
#include <libgds/fifo.h>
#include <libgds/memory.h>
#include <net/network.h>
#include <net/net_path.h>
#include <sim/simulator.h>
#include <libgds/str_util.h>

#ifdef HAVE_LIBREADLINE
#ifdef HAVE_READLINE_READLINE_H
#include <readline/readline.h>
#define INTERACTIVE_MODE_OK
#endif
#include <ui/rl.h>
#endif

// ----- global options -----
char * pcOptCli         = NULL;
char * pcOptLog         = NULL;
int iOptInteractive     = 0;

// ----- fatal_error ------------------------------------------------
/**
 *
 */
void fatal_error(char * pcMessage)
{
  LOG_FATAL("FATAL: %s\n", pcMessage);
  exit(EXIT_FAILURE);
}

// ----- simulation_init --------------------------------------------
/**
 *
 */
void simulation_init()
{
  if (iOptInteractive)
    fprintf(stdout, "cbgp> init.\n");
  simulator_init();
}

// ----- simulation_done --------------------------------------------
/**
 *
 */
void simulation_done()
{
  if (iOptInteractive)
    fprintf(stdout, "cbgp> done.\n");
  simulator_done();
}

// ----- rl_display_completion --------------------------------------
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
#ifdef INTERACTIVE_MODE_OK
void rl_display_completion(char **matches, int num_matches, int max_length)
{
  int iIndex;

  fprintf(stdout, "\n");
  for (iIndex= 1; iIndex <= num_matches; iIndex++) {
    fprintf(stdout, "\t%s\n", matches[iIndex]);
  }
  rl_on_new_line();
}
#endif

// ----- completion state -----
SCliCmd * pComplCmd= NULL;    // Current command
SCliCmd * pComplCtxCmd= NULL; // Context command
SCliCmds * pComplCmds= NULL;  // List of subcmds
int iComplParamIndex= 0;      // Index of current parameter

// ----- rl_compl_cmd_generator -------------------------------------
/**
 * This function generates completion for CLI commands. It relies on
 * the list of sub-commands of the current command.
 */
#ifdef INTERACTIVE_MODE_OK
#ifdef HAVE_RL_COMPLETION_MATCHES
char * rl_compl_cmd_generator(const char * pcText, int iState)
{
  static int iIndex;
  char * pcMatch= NULL;
  char * pcCmdName;

  /* Check that a command is to be completed */
  if (pComplCmd == NULL)
    return NULL;

  /* First call, initialize generator's state */
  if (iState == 0) {
    if (pComplCmds != NULL)
      cli_cmds_destroy(&pComplCmds);
    pComplCmds= cli_matching_cmds(pComplCmd->pSubCmds, pcText);
    iIndex= 0;
  }

  /* Obtain matching commands... */
  if ((pComplCmds != NULL) && (iIndex < ptr_array_length(pComplCmds))) {
    pcCmdName= ((SCliCmd *) pComplCmds->data[iIndex])->pcName;
    pcMatch= (char*) malloc((strlen(pcCmdName)+1)*sizeof(char));
    strcpy(pcMatch, pcCmdName);
    iIndex++;
    rl_attempted_completion_over= 0;
  } else {
    rl_attempted_completion_over= 1;
  }

  return pcMatch;
}
#endif
#endif

// ----- rl_compl_param_generator -----------------------------------
/**
 * This function generates completion for CLI parameter values. The
 * generator first relies on a possible enumeration function [not
 * implemented yet], then on a parameter checking function.
 */
#ifdef INTERACTIVE_MODE_OK
#ifdef HAVE_RL_COMPLETION_MATCHES
char * rl_compl_param_generator(const char * pcText, int iState)
{
  static int iIndex;
  char * pcMatch= NULL;
  SCliCmdParam * pParam;

  /* Check that a command and a valid parameter are to be completed */
  if ((pComplCmd == NULL) || (pComplCmd->pParams == NULL) ||
      (iComplParamIndex >= cli_cmd_get_num_params(pComplCmd)))
    return NULL;

  /* Complete if possible */

  pParam= (SCliCmdParam *) pComplCmd->pParams->data[iComplParamIndex];

  /* If the parameter supports a completion function */
  if (pParam->fEnumParam != NULL) {
    if (iState == 0) {
      iIndex= 0;
    }
    pcMatch= pParam->fEnumParam(pcText, iIndex);
    iIndex++;
  } else {
    if (iState == 0) {
      if ((pParam->fCheckParam != NULL) &&
	  (pParam->fCheckParam(pcText))) {

	/* If there is a parameter checker and the parameter value is
	   accepted, perform the completion */
	pcMatch= (char*) malloc((strlen(pcText)+1)*sizeof(char));
	strcpy(pcMatch, pcText);
	rl_attempted_completion_over= 1;
      } else {
	/* Otherwize, prevent the completion */
	rl_attempted_completion_over= 1;
      }
    }     
  }
    
  return pcMatch;  
}
#endif
#endif

// ----- rl_compl ---------------------------------------------------
/**
 *
 */
#ifdef INTERACTIVE_MODE_OK
#ifdef HAVE_RL_COMPLETION_MATCHES
char ** rl_compl (const char * pcText, int iStart, int iEnd)
{
  SCli * pCli= cli_get();
  char * pcStartCmd;
  char ** apcMatches= NULL;
  int iParamIndex;
  SCliCtxItem * pCtxItem;

  /* Authorize readline's completion */
  rl_attempted_completion_over= 0;

  /* Find context command */
  pComplCtxCmd= NULL;
  if (pCli->pExecContext != NULL) {
    pCtxItem= cli_context_top(pCli->pExecContext);
    if (pCtxItem != NULL)
      pComplCtxCmd= pCtxItem->pCmd;
  }
  if (pComplCtxCmd == NULL)
    pComplCtxCmd= pCli->pBaseCommand;
  iComplParamIndex= 0;
  
  /* Match beginning of readline buffer */
  if (iStart == 0) {
    pComplCmd= pComplCtxCmd;
  } else {
    pcStartCmd= (char *) malloc((iStart+1)*sizeof(char));
    strncpy(pcStartCmd, rl_line_buffer, iStart);
    pcStartCmd[iStart]= '\0';
    pComplCmd= cli_cmd_match_subcmds(pCli, pComplCtxCmd,
				     pcStartCmd, &iParamIndex);
    free(pcStartCmd);
    iComplParamIndex= iParamIndex;
  }

  /* Depending on state, call cmd/param generator */
  if ((pComplCmd != NULL) && ((pComplCmd->pSubCmds != NULL) ||
			      (pComplCmd->pParams != NULL))) {
    if ((iStart > 0) &&
	(iComplParamIndex < cli_cmd_get_num_params(pComplCmd))) {
      apcMatches= rl_completion_matches(pcText, rl_compl_param_generator);
    } else {
      apcMatches= rl_completion_matches(pcText, rl_compl_cmd_generator);
    }
  } else {
    rl_attempted_completion_over= 1;
  }

  return apcMatches;
}
#endif
#endif

// ----- simulation_help_ctx ----------------------------------------
/**
 *
 */
#ifdef INTERACTIVE_MODE_OK
void simulation_help_ctx(SCliCmd * pCtxCmd)
{
  int iIndex, iIndex2;
  SCliCmd * pCmd;
  SCliCmdParam * pParam;

  fprintf(stdout, "AVAILABLE COMMANDS:\n");

  for (iIndex= 0; iIndex < cli_cmd_get_num_subcmds(pCtxCmd); iIndex++) {
    pCmd= (SCliCmd *) pCtxCmd->pSubCmds->data[iIndex];
    fprintf(stdout, "\t%s", pCmd->pcName);
    for (iIndex2= 0; iIndex2 < cli_cmd_get_num_params(pCmd); iIndex2++) {
      pParam= (SCliCmdParam *) pCmd->pParams->data[iIndex2];
      fprintf(stdout, " %s", pParam->pcName);
    }
    fprintf(stdout, "\n");
  }
  rl_on_new_line();
}
#endif

// ----- simulation_help_cmd ----------------------------------------
/**
 *
 */
#ifdef INTERACTIVE_MODE_OK
void simulation_help_cmd(SCliCmd * pCmd, int iParamIndex)
{
  int iIndex;
  SCliCmdParam * pParam;

  fprintf(stdout, "SYNTAX:\n");

  fprintf(stdout, "\t%s", pCmd->pcName);

  for (iIndex= 0; iIndex < cli_cmd_get_num_params(pCmd); iIndex++) {
    pParam= (SCliCmdParam *) pCmd->pParams->data[iIndex];
    fprintf(stdout, " %s", pParam->pcName);
  }
  fprintf(stdout, "\n");

  if (pCmd->pcHelp != NULL) {
    fprintf(stdout, "\nCOMMENTS:\n%s\n", pCmd->pcHelp);
  }
  rl_on_new_line();
}
#endif

// ----- simulation_help --------------------------------------------
/**
 *
 */
#ifdef INTERACTIVE_MODE_OK
void simulation_help(const char * pcLine)
{
  SCli * pCli= cli_get();
  SCliCmd * pCtxCmd= NULL;
  SCliCtxItem * pCtxItem;
  SCliCmd * pCmd= NULL;
  int iParamIndex;
  char * pcStartCmd;
  char * pcPos;

  /* Find context command */
  pCtxCmd= NULL;
  if (pCli->pExecContext != NULL) {
    pCtxItem= cli_context_top(pCli->pExecContext);
    if (pCtxItem != NULL) {
      pCtxCmd= pCtxItem->pCmd;
    }
  }
  if (pCtxCmd == NULL)
    pCtxCmd= pCli->pBaseCommand;

  /* Match beginning of readline buffer */
  pcPos= strchr(pcLine, '?');
  if ((int) (pcPos-pcLine) > 0) {
    pcStartCmd= (char *) malloc((((int) (pcPos-pcLine))+1)*sizeof(char));
    strncpy(pcStartCmd, rl_line_buffer, (int) (pcPos-pcLine));
    pcStartCmd[(int) (pcPos-pcLine)]= '\0';
    pCmd= cli_cmd_match_subcmds(pCli, pCtxCmd,
				pcStartCmd, &iParamIndex);
    free(pcStartCmd);
  } else {
    pCmd= pCtxCmd;
    iParamIndex= 0;
  }
  
  if (pCmd != NULL) {
    if (pCmd == pCtxCmd) {
      simulation_help_ctx(pCtxCmd);
    } else if ((pCmd->pParams != NULL) &&
	       (iParamIndex < cli_cmd_get_num_params(pCmd))) {
      simulation_help_cmd(pCmd, iParamIndex);
    } else {
      simulation_help_ctx(pCmd);
    }
  } else {
    fprintf(stdout, "Sorry, no help is available on this topic\n");
    rl_on_new_line();
  }
}
#endif

// ----- simulation_interactive -------------------------------------
/**
 *
 */
int simulation_interactive()
{
#ifdef INTERACTIVE_MODE_OK
  int iResult= CLI_SUCCESS;
  char * pcLine;

#ifdef HAVE_RL_COMPLETION_MATCHES
  /* Setup alternate completion function */
  rl_attempted_completion_function= rl_compl;

  /* Setup alternate completion display function */
  rl_completion_display_matches_hook= rl_display_completion;
#endif

  while (1) {
    /* Get user-input */
    pcLine= rl_gets(cli_context_to_string(cli_get()->pExecContext,
					  "cbgp"));
    /* EOF has been catched (Ctrl-D), exit */
    if (pcLine == NULL) {
      fprintf(stdout, "\n");
      break;
    }

    /* Execute command */
    iResult= cli_execute_line(cli_get(), pcLine);

    if (iResult == CLI_SUCCESS_TERMINATE)
      break;

    if (iResult == CLI_SUCCESS_HELP) {
      simulation_help(pcLine);
    }

  }
  return iResult;
#else
  fprintf(stderr, "Error: compiled without GNU readline.\n");
  return -1;
#endif
}

// ----- main_help --------------------------------------------------
/**
 *
 */
void main_help()
{
  printf("C-BGP v1.1\n");
  printf("Infonet Group, CSE Dept, UCL, Belgium\n");
  printf("\n");
  printf("Usage: cbgp [OPTIONS]\n");
  printf("\n");
  printf("  -h             display this message.\n");
  printf("  -i             interactive mode.\n");
  printf("  -l LOGFILE     output log to LOGFILE instead of stderr.\n");
  printf("  -c SCRIPT      load and execute SCRIPT file.\n");
  printf("                 (without this option, commands are taken from stdin)\n");
#if defined(HAVE_MEM_FLAG_SET) && (HAVE_MEM_FLAG_SET == 1)
  printf("  -g             track memory leaks.\n");
#endif
  printf("\n");
}

// ----- option_string ----------------------------------------------
/**
 *
 */
char * option_string(char * pcArgument)
{
  return str_create(pcArgument);
}

// ----- option_free ------------------------------------------------
/**
 *
 */
void option_free(char * pcArgument)
{
  if (pcArgument != NULL)
    str_destroy(&pcArgument);
}

typedef struct {
  uint16_t uAS;
  SPath * pPath;
  net_link_delay_t tDelay;
} SContext;

// ----- as_shortest_path ----------------------------------------------
/**
 * Calculate shortest path from the given source AS towards all the
 * other ASes.
 */
/*int as_shortest_path(FILE * pStream, uint16_t uSrcAS)
{
  SAS * pAS;
  SPeer * pPeer;
  SFIFO * pFIFO;
  uint16_t auVisited[MAX_AS];
  SPath * pPath;
  SContext * pContext, * pOldContext;
  int iIndex;
  
  memset(auVisited, 0, sizeof(auVisited));
  pFIFO= fifo_create(100000, NULL);
  pContext= (SContext *) MALLOC(sizeof(SContext));
  pContext->uAS= uSrcAS;
  pContext->pPath= path_create();
  fifo_push(pFIFO, pContext);
  auVisited[uSrcAS]= 1;

  // Breadth-first search
  while (1) {
    pContext= (SContext *) fifo_pop(pFIFO);
    if (pContext == NULL)
      break;
    pAS= AS[pContext->uAS];
    fprintf(pStream, "AS%d\tAS%d\t", uSrcAS, pContext->uAS);
    pPath= pContext->pPath;
    path_dump(pStream, pPath, 0);
    fprintf(pStream, "\n");

    pOldContext= pContext;

    for (iIndex= 0; iIndex < ptr_array_length(pAS->pPeers);
	 iIndex++) {
      pPeer= (SPeer *) pAS->pPeers->data[iIndex];
      if (auVisited[pPeer->uRemoteAS] == 0) {
	auVisited[pPeer->uRemoteAS]= 1;
	pContext= (SContext *) MALLOC(sizeof(SContext));
	pContext->uAS= pPeer->uRemoteAS;
	pContext->pPath= path_copy(pPath);
	path_append(&pContext->pPath, pPeer->uRemoteAS);
	assert(fifo_push(pFIFO, pContext) == 0);
      }
    }
    path_destroy(&pOldContext->pPath);
    FREE(pOldContext);
  }
  fifo_destroy(&pFIFO);
  return 0;
}*/

// ----- as_dijkstra -------------------------------------------
/**
 * Calculate shortest path from the given source AS towards all the
 * other ASes.
 */
/*int as_dijkstra(FILE * pStream, uint16_t uSrcAS)
{
  SAS * pAS;
  SPeer * pPeer;
  SFIFO * pFIFO;
  net_link_delay_t atVisited[MAX_AS];
  SPath * pPath;
  SContext * pContext, * pOldContext;
  int iIndex;
  
  for (iIndex= 0; iIndex < MAX_AS; iIndex++)
    atVisited[iIndex]= NET_LINK_DELAY_INFINITE;
  pFIFO= fifo_create(100000, NULL);
  pContext= (SContext *) MALLOC(sizeof(SContext));
  pContext->uAS= uSrcAS;
  pContext->pPath= path_create();
  fifo_push(pFIFO, pContext);
  atVisited[uSrcAS]= 0;

  // Breadth-first search
  while (1) {
    pContext= (SContext *) fifo_pop(pFIFO);
    if (pContext == NULL)
      break;
    pAS= AS[pContext->uAS];
    fprintf(pStream, "AS%d\tAS%d\t", uSrcAS, pContext->uAS);
    pPath= pContext->pPath;
    path_dump(pStream, pPath, 0);
    fprintf(pStream, "\n");

    pOldContext= pContext;

    for (iIndex= 0; iIndex < ptr_array_length(pAS->pPeers);
	 iIndex++) {
      pPeer= (SPeer *) pAS->pPeers->data[iIndex];
      if (atVisited[pPeer->uRemoteAS] > 0) {
	atVisited[pPeer->uRemoteAS]= 0;
	pContext= (SContext *) MALLOC(sizeof(SContext));
	pContext->uAS= pPeer->uRemoteAS;
	pContext->pPath= path_copy(pPath);
	path_append(&pContext->pPath, pPeer->uRemoteAS);
	assert(fifo_push(pFIFO, pContext) == 0);
      }
    }
    path_destroy(&pOldContext->pPath);
    FREE(pOldContext);
  }
  fifo_destroy(&pFIFO);
  return 0;
}*/

// ----- simulation_load_sp -----------------------------------------
/**
 *
 */
/*int simulation_load_sp(char * pcFileName)
{
  FILE * pFileInput, * pFileOutput;
  char * pcOutFileName;
  int iError= 0;
  uint16_t uAS;
  char acFileLine[80];

  pcOutFileName= (char *) MALLOC(sizeof(char)*(strlen(pcFileName)+5));
  strcpy(pcOutFileName, pcFileName);
  strcat(pcOutFileName, "-out");

  printf("simulator> load \"%s\"\n", pcFileName);
  printf("simulator> write \"%s\"\n", pcOutFileName);
  if ((pFileOutput= fopen(pcOutFileName, "w")) != NULL) {
    if ((pFileInput= fopen(pcFileName, "r")) != NULL) {
      while ((!feof(pFileInput)) && (!iError)) {
	if (fgets(acFileLine, sizeof(acFileLine), pFileInput) == NULL)
	  break;
	if (sscanf(acFileLine, "* %hu", &uAS) != 1) {
	  iError= 1;
	  break;
	}
	
	// Check that the destination AS number is valid
	if ((uAS > MAX_AS) || (AS[uAS] == NULL)) {
	  iError= 1;
	  break;
	}
	
	// Calculate shortest path
	as_shortest_path(pFileOutput, uAS);
	
      }
      fclose(pFileInput);
    } else
      iError= 1;
    fclose(pFileOutput);
  } else
    iError= 1;
  FREE(pcOutFileName);
  if (iError)
    return -1;
  return 0;
}*/

/////////////////////////////////////////////////////////////////////
// INITIALIZATION AND FINALIZATION SECTION
/////////////////////////////////////////////////////////////////////

void _main_done() __attribute((destructor));

void _main_done()
{
  option_free(pcOptCli);
  option_free(pcOptLog);
  if (pComplCmds != NULL)
    cli_cmds_destroy(&pComplCmds);
}

/////////////////////////////////////////////////////////////////////
// MAIN PROGRAM
/////////////////////////////////////////////////////////////////////

// ----- main -------------------------------------------------------
/**
 *
 */
int main(int argc, char ** argv) {
  int iResult;
  FILE * pInCli;
  int iExitCode= EXIT_SUCCESS;

  while ((iResult= getopt(argc, argv, "c:hil:g")) != -1) {
    switch (iResult) {
    case 'c':
      pcOptCli= option_string(optarg);
      break;
    case 'g':
#if defined(HAVE_MEM_FLAG_SET) && (HAVE_MEM_FLAG_SET == 1)
      mem_flag_set(MEM_FLAG_WARN_LEAK, 1);
#else
      fprintf(stderr, "Error: option -g not supported (check your libgds version).\n");
      exit(EXIT_FAILURE);
#endif
      break;
    case 'h':
      main_help();
      exit(EXIT_SUCCESS);
      break;
    case 'i':
      iOptInteractive= 1;
      break;
    case 'l':
      pcOptLog= option_string(optarg);
      break;
    default:
      main_help();
      exit(EXIT_FAILURE);
    }
  }

  // Setup log
  log_set_level(pMainLog, LOG_LEVEL_WARNING);
  if (pcOptLog)
    log_set_file(pMainLog, pcOptLog);
  else
    log_set_stream(pMainLog, stderr);

  simulation_init();

    // Run Cli-script
  if (pcOptCli != NULL) {
    pInCli= fopen(pcOptCli, "r");
    if (pInCli != NULL) {
      if (cli_execute_file(cli_get(), pInCli) != CLI_SUCCESS)
	iExitCode= EXIT_FAILURE;
      fclose(pInCli);
    } else {
      LOG_SEVERE("Error: Unable to open script file \"%s\"\n", pcOptCli);
      iExitCode= EXIT_FAILURE;
    }
  } else {
    if (iOptInteractive) {
      if (simulation_interactive() != CLI_SUCCESS)
	iExitCode= EXIT_FAILURE;
    } else
      if (cli_execute_file(cli_get(), stdin) != CLI_SUCCESS)
	iExitCode= EXIT_FAILURE;
  }

  simulation_done();

  return iExitCode;
}
