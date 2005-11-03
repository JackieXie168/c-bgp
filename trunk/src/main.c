// ==================================================================
// @(#)main.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @author Sebastien Tandel (standel@info.ucl.ac.be)
// @date 22/11/2002
// @lastdate 14/10/2005
// 
// @LICENSE@
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <assert.h>
#include <libgds/cli_ctx.h>
#include <libgds/gds.h>
#include <limits.h>
#include <libgds/log.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <bgp/as.h>
#include <bgp/comm_hash.h>
#include <bgp/domain.h>
#include <bgp/filter_registry.h>
#include <bgp/mrtd.h>
#include <bgp/path_hash.h>
#include <bgp/peer_t.h>
#include <bgp/predicate_parser.h>
#include <bgp/qos.h>
#include <bgp/rexford.h>
#include <bgp/route_map.h>
#include <cli/common.h>
#include <libgds/fifo.h>
#include <libgds/memory.h>
#include <net/igp_domain.h>
#include <net/network.h>
#include <net/net_path.h>
#include <sim/simulator.h>
#include <libgds/str_util.h>

#include <net/ospf.h>

#ifdef HAVE_LIBREADLINE
# ifdef HAVE_READLINE_READLINE_H
#  include <readline/readline.h>
# define INTERACTIVE_MODE_OK
# endif
# include <ui/rl.h>
#endif

// -----[ simulator's modes] -----
#define CBGP_MODE_DEFAULT     0
#define CBGP_MODE_INTERACTIVE 1
#define CBGP_MODE_SCRIPT      2
#define CBGP_MODE_EXECUTE     3
#define CBGP_MODE_OSPF        4

// -----[ global options ]-----
char * pcOptLog = NULL;
uint8_t uMode   = CBGP_MODE_DEFAULT;
char * pcArgMode= NULL;

// -----[ simulation_cli_help ]--------------------------------------
/**
 * This function shows a command-line usage screen.
 */
void simulation_cli_help()
{
  printf("C-BGP %s ", PACKAGE_VERSION);
  printf("Copyright (C) 2005 Bruno Quoitin\n");
  printf("Networking group, CSE Dept, UCL, Belgium\n");
  printf("\n");
  printf("Usage: cbgp [OPTIONS]\n");
  printf("\n");
  printf("  -h             display this message.\n");
  printf("  -i             interactive mode.\n");
  printf("  -l LOGFILE     output log to LOGFILE instead of stderr.\n");
  printf("  -c SCRIPT      load and execute SCRIPT file.\n");
  printf("                 (without this option, commands are taken from stdin)\n");
  printf("  -e COMMAND     execute the given command\n");
#if defined(HAVE_MEM_FLAG_SET) && (HAVE_MEM_FLAG_SET == 1)
  printf("  -g             track memory leaks.\n");
#endif

#ifdef OSPF_SUPPORT
  printf("  -o             test OSPF model (cbgp must be compiled with --enable-ospf option).\n");
#endif
  printf("\n");
  printf("C-BGP comes with ABSOLUTELY NO WARRANTY.\n");
  printf("This is free software, and you are welcome to redistribute it\n");
  printf("under certain conditions; see file COPYING for details.\n");
  printf("\n");
}

// -----[ simulation_set_mode ]--------------------------------------
/**
 * Change the simulation mode.
 */
void simulation_set_mode(uint8_t uNewMode, char * pcNewArg)
{
  if (uMode == CBGP_MODE_DEFAULT) {
    uMode= uNewMode;
    if (pcNewArg != NULL)
      pcArgMode= pcNewArg;
  } else {
    fprintf(stderr, "Error: multiple modes specified on command-line.\n");
    simulation_cli_help();
    exit(EXIT_FAILURE);
  }
}

// -----[ simulation_init ]------------------------------------------
/**
 * initilize the simulation.
 */
void simulation_init()
{
  if (uMode == CBGP_MODE_INTERACTIVE)
    fprintf(stdout, "cbgp> init.\n");
  simulator_init();
}

// ----- simulation_done --------------------------------------------
/**
 * Finalize the simulation.
 */
void simulation_done()
{
  if (uMode == CBGP_MODE_INTERACTIVE)
    fprintf(stdout, "cbgp> done.\n");
  simulator_done();
}

/////////////////////////////////////////////////////////////////////
//
// COMMAND COMPLETION
//
/////////////////////////////////////////////////////////////////////

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
 *
 * Note: use malloc() for the returned matches, not MALLOC().
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
 *
 * Note: use malloc() for the returned matches, not MALLOC().
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

/////////////////////////////////////////////////////////////////////
//
// ONLINE HELP
//
/////////////////////////////////////////////////////////////////////

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

  if (cli_cmd_get_num_subcmds(pCtxCmd) > 0) {
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

  } else {
    fprintf(stdout, "NO HELP AVAILABLE.\n");
  }

  /* Tell readline that we have written things. The command prompt
     must therefore start on a new line. */
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

/////////////////////////////////////////////////////////////////////
// INITIALIZATION AND FINALIZATION SECTION
/////////////////////////////////////////////////////////////////////

void _main_init() __attribute((constructor));
void _main_done() __attribute((destructor));

void _main_init()
{
  gds_init(0);
  //gds_init(GDS_OPTION_MEMORY_DEBUG);

  // Hash init code commented in order to allow parameter setup
  // through he command-line/script
  //_comm_hash_init();
  //_path_hash_init();

  _network_create();
  _bgp_domain_init();
  _igp_domain_init();
  _ft_registry_init();
  _filter_path_regex_init();
  _route_map_init();
  _rexford_init();
#ifdef HAVE_LIBREADLINE
  _rl_init();
#endif
}

void _main_done()
{
  option_free(pcArgMode);
  option_free(pcOptLog);
  if (pComplCmds != NULL)
    cli_cmds_destroy(&pComplCmds);

  _cli_common_destroy();
#ifdef HAVE_LIBREADLINE
  _rl_destroy();
#endif
  _message_destroy();
  _rexford_destroy();
  _route_map_destroy();
  _filter_path_regex_destroy();
  _ft_registry_destroy();
  _bgp_domain_destroy();
  _igp_domain_destroy();
  _network_destroy();
  _mrtd_destroy();
  _path_hash_destroy();
  _comm_hash_destroy();

  gds_destroy();
}

/////////////////////////////////////////////////////////////////////
// MAIN PROGRAM
/////////////////////////////////////////////////////////////////////

// ----- main -------------------------------------------------------
/**
 * This is C-BGP's main entry point.
 */
int main(int argc, char ** argv) {
  int iResult;
  FILE * pInCli;
  int iExitCode= EXIT_SUCCESS;

  /* Initialize log */
  log_set_level(pMainLog, LOG_LEVEL_WARNING);
  log_set_stream(pMainLog, stderr);

  /* Process command-line options */
  while ((iResult= getopt(argc, argv, "c:e:hil:go")) != -1) {
    switch (iResult) {
    case 'c':
      simulation_set_mode(CBGP_MODE_SCRIPT, option_string(optarg));
      break;
    case 'e':
      simulation_set_mode(CBGP_MODE_EXECUTE, option_string(optarg));
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
      simulation_cli_help();
      exit(EXIT_SUCCESS);
      break;
    case 'i':
      simulation_set_mode(CBGP_MODE_INTERACTIVE, NULL);
      break;
    case 'l':
      pcOptLog= option_string(optarg);
      break;
#ifdef OSPF_SUPPORT
    case 'o':  //only to test ospf function
      simulation_set_mode(CBGP_MODE_OSPF, NULL);
      break;
#endif
    default:
      simulation_cli_help();
      exit(EXIT_FAILURE);
    }
  }

  /* Setup log stream */
  if (pcOptLog)
    log_set_file(pMainLog, pcOptLog);

  simulation_init();

  /* Run simulation in selected mode... */
  switch (uMode) {
  case CBGP_MODE_DEFAULT:
    if (cli_execute_file(cli_get(), stdin) != CLI_SUCCESS)
      iExitCode= EXIT_FAILURE;
    break;
  case CBGP_MODE_INTERACTIVE:
    if (simulation_interactive() != CLI_SUCCESS)
      iExitCode= EXIT_FAILURE;
    break;
  case CBGP_MODE_SCRIPT:
    pInCli= fopen(pcArgMode, "r");
    if (pInCli != NULL) {
      if (cli_execute_file(cli_get(), pInCli) != CLI_SUCCESS) {
	iExitCode= EXIT_FAILURE;
      }
      fclose(pInCli);
    } else {
      LOG_SEVERE("Error: Unable to open script file \"%s\"\n", pcArgMode);
      iExitCode= EXIT_FAILURE;
    }
    break;
  case CBGP_MODE_EXECUTE:
    iExitCode= (cli_execute_line(cli_get(), pcArgMode) == 0)?
      EXIT_SUCCESS:EXIT_FAILURE;
    break;

  case CBGP_MODE_OSPF:
#ifdef OSPF_SUPPORT
    iExitCode= ospf_test();
    if (iExitCode != 0)
      printf("ospf_test ok!\n");
    else 
      printf("ospf_test has failed :-(\n");
#else
    printf("You must compile cbgp with --enable-ospf option to use OSPF model");
#endif
    break;
  }
  
  simulation_done();
  return iExitCode;
}
