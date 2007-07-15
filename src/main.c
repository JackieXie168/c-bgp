// ==================================================================
// @(#)main.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @author Sebastien Tandel (standel@info.ucl.ac.be)
// @date 22/11/2002
// @lastdate 22/06/2007
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "api.h"

#include <libgds/memory.h>
#include <libgds/str_util.h>

#include <ui/rl.h>
#include <ui/help.h>
#include <cli/common.h>

//#include <net/ospf.h>

// -----[ simulator's modes] -----
#define CBGP_MODE_DEFAULT     0
#define CBGP_MODE_INTERACTIVE 1
#define CBGP_MODE_SCRIPT      2
#define CBGP_MODE_EXECUTE     3
#define CBGP_MODE_OSPF        4

// -----[ global options ]-----
uint8_t uMode   = CBGP_MODE_DEFAULT;
char * pcArgMode= NULL;

// -----[ simulation_cli_help ]--------------------------------------
/**
 * This function shows a command-line usage screen.
 */
void simulation_cli_help()
{
  printf("C-BGP routing solver %s\n", PACKAGE_VERSION);
  printf("Copyright (C) 2007 Bruno Quoitin\n");
  printf("IP Networking Lab, CSE Dept, UCL, Belgium\n");
  printf("\nUsage: cbgp [OPTIONS]\n");
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

// ----- simulation_interactive -------------------------------------
/**
 *
 */
int simulation_interactive()
{
  int iResult= CLI_SUCCESS;
  char * pcLine;

  fprintf(stdout, "C-BGP routing solver %s\n", PACKAGE_VERSION);
  fprintf(stdout, "Copyright (C) 2007 Bruno Quoitin\n");
  fprintf(stdout, "IP Networking Lab, CSE Dept, UCL, Belgium\n");
  fprintf(stdout, "\n");
  fprintf(stdout, "cbgp> init.\n");

  _rl_init();

  while (1) {
    /* Get user-input */
    pcLine= rl_gets();

    /* EOF has been catched (Ctrl-D), exit */
    if (pcLine == NULL) {
      fprintf(stdout, "\n");
      break;
    }

    /* Execute command */
    iResult= libcbgp_exec_cmd(pcLine);

    if (iResult == CLI_SUCCESS_TERMINATE)
      break;

  }

  _rl_destroy();

  fprintf(stdout, "cbgp> done.\n");

  return iResult;
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
  libcbgp_init();
}

void _main_done()
{
  option_free(pcArgMode);
  libcbgp_done();
}

/////////////////////////////////////////////////////////////////////
//
// MAIN PROGRAM
//
/////////////////////////////////////////////////////////////////////

// ----- main -------------------------------------------------------
/**
 * This is C-BGP's main entry point.
 */
int main(int argc, char ** argv) {
  int iResult;
  int iExitCode= EXIT_SUCCESS;

  /* Process command-line options */
  while ((iResult= getopt(argc, argv, "mc:e:hil:got:")) != -1) {
    switch (iResult) {
    case 'c':
      simulation_set_mode(CBGP_MODE_SCRIPT, option_string(optarg));
      break;
    case 'e':
      simulation_set_mode(CBGP_MODE_EXECUTE, option_string(optarg));
      break;
    case 'g':
#if defined(HAVE_MEM_FLAG_SET) && (HAVE_MEM_FLAG_SET == 1)
      //mem_flag_set(MEM_FLAG_WARN_LEAK, 1);
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
      libcbgp_set_debug_file(optarg);
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

  /* Run simulation in selected mode... */
  switch (uMode) {
  case CBGP_MODE_DEFAULT:
    if (libcbgp_exec_stream(stdin) != CLI_SUCCESS)
      iExitCode= EXIT_FAILURE;
    break;
  case CBGP_MODE_INTERACTIVE:
    if (simulation_interactive() != CLI_SUCCESS)
      iExitCode= EXIT_FAILURE;
    break;
  case CBGP_MODE_SCRIPT:
    iExitCode= (libcbgp_exec_file(pcArgMode) == 0)?
      EXIT_SUCCESS:EXIT_FAILURE;
    break;
  case CBGP_MODE_EXECUTE:
    iExitCode= (libcbgp_exec_cmd(pcArgMode) == 0)?
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
  
  return iExitCode;
}
