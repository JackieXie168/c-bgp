// ==================================================================
// @(#)main.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be), Sebastien Tandel
// @date 22/11/2002
// @lastdate 01/06/2004
// ==================================================================

#include <config.h>
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

// ----- simulation_interactive -------------------------------------
/**
 *
 */
int simulation_interactive()
{
#ifdef INTERACTIVE_MODE_OK
  int iResult= CLI_SUCCESS;
  char * pcLine;

  while (1) {
    // Get user-input
    pcLine= rl_gets(cli_context_to_string(cli_get()->pExecContext,
					  "cbgp"));
    // EOF has been catched (Ctrl-D), exit
    if (pcLine == NULL) {
      fprintf(stdout, "\n");
      break;
    }

    // Execute command
    iResult= cli_execute_line(cli_get(), pcLine);
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
      fprintf(stderr, "Error: option -g not supported (check you libgds version).\n");
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