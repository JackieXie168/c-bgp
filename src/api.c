// ==================================================================
// @(#)api.c
//
// Application Programming Interface for the C-BGP library (libcsim).
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 25/10/2006
// $Id: api.c,v 1.13 2008-06-12 14:23:51 bqu Exp $
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <api.h>

#include <libgds/gds.h>

#include <bgp/as.h>
#include <bgp/as-level.h>
#include <bgp/comm.h>
#include <bgp/comm_hash.h>
#include <bgp/domain.h>
#include <bgp/filter_registry.h>
#include <bgp/mrtd.h>
#include <bgp/path.h>
#include <bgp/path_hash.h>
#include <bgp/path_segment.h>
#include <bgp/peer_t.h>
#include <bgp/predicate_parser.h>
#include <bgp/qos.h>
#include <bgp/rexford.h>
#include <bgp/route_map.h>
#include <cli/common.h>
#include <net/igp_domain.h>
#include <sim/simulator.h>
#include <ui/rl.h>



/////////////////////////////////////////////////////////////////////
//
// Initialization and configuration of the library.
//
/////////////////////////////////////////////////////////////////////

// -----[ libcbgp_init ]---------------------------------------------
/**
 * Initialize the C-BGP library.
 */
CBGP_EXP_DECL
void libcbgp_init()
{
#ifdef __MEMORY_DEBUG__
  gds_init(GDS_OPTION_MEMORY_DEBUG);
#else
  gds_init(0);
#endif
  libcbgp_init2();
}

// -----[ libcbgp_done ]---------------------------------------------
/**
 * Free the resources allocated by the C-BGP library.
 */
CBGP_EXP_DECL
void libcbgp_done()
{
  libcbgp_done2();
  gds_destroy();
}

// -----[ libcbgp_banner ]-------------------------------------------
/**
 *
 */
CBGP_EXP_DECL
void libcbgp_banner()
{
  log_printf(pLogOut, "C-BGP routing solver %s\n", PACKAGE_VERSION);
  log_printf(pLogOut, "Copyright (C) 2008 Bruno Quoitin\n");
  log_printf(pLogOut, "IP Networking Lab, CSE Dept, UCL, Belgium\n");
  log_printf(pLogOut, "\n");
  log_printf(pLogOut, "C-BGP comes with ABSOLUTELY NO WARRANTY.\n");
  log_printf(pLogOut, "This is free software, and you are welcome to redistribute it\n");
  log_printf(pLogOut, "under certain conditions; see file COPYING for details.\n");
  log_printf(pLogOut, "\n");
}

// -----[ libcbgp_set_debug_callback ]-------------------------------
/**
 *
 */
CBGP_EXP_DECL
void libcbgp_set_debug_callback(FLogStreamCallback fCallback,
				void * pContext)
{
  SLogStream * pLogTmp= log_create_callback(fCallback, pContext);
  
  if (pLogTmp == NULL) {
    fprintf(stderr, "Warning: couln't direct debug log to callback %p.\n",
	    fCallback);
    return;
  }

  log_destroy(&pLogDebug);
  pLogDebug= pLogTmp;
}

// -----[ libcbgp_set_err_callback ]---------------------------------
/**
 *
 */
CBGP_EXP_DECL
void libcbgp_set_err_callback(FLogStreamCallback fCallback,
			      void * pContext)
{
  SLogStream * pLogTmp= log_create_callback(fCallback, pContext);
  
  if (pLogTmp == NULL) {
    fprintf(stderr, "Warning: couln't direct error log to callback %p.\n",
	    fCallback);
    return;
  }

  log_destroy(&pLogErr);
  pLogErr= pLogTmp;
}

// -----[ libcbgp_set_out_callback ]---------------------------------
/**
 *
 */
CBGP_EXP_DECL
void libcbgp_set_out_callback(FLogStreamCallback fCallback,
			      void * pContext)
{
  SLogStream * pLogTmp= log_create_callback(fCallback, pContext);

  if (pLogTmp == NULL) {
    fprintf(stderr, "Warning: couln't direct output to callback %p.\n",
	    fCallback);
    return;
  }

  log_destroy(&pLogOut);
  pLogOut= pLogTmp;
}

// -----[ libcbgp_set_debug_level ]----------------------------------
/**
 *
 */
CBGP_EXP_DECL
void libcbgp_set_debug_level(ELogLevel eLevel)
{
  log_set_level(pLogDebug, eLevel);
}

// -----[ libcbgp_set_err_level ]------------------------------------
/**
 *
 */
CBGP_EXP_DECL
void libcbgp_set_err_level(ELogLevel eLevel)
{
  log_set_level(pLogErr, eLevel);
}

// -----[ libcbgp_set_debug_file ]-----------------------------------
/**
 *
 */
CBGP_EXP_DECL
void libcbgp_set_debug_file(char * pcFileName)
{
  SLogStream * pLogTmp= log_create_file(pcFileName);

  if (pLogTmp == NULL) {
    fprintf(stderr, "Warning: couln't direct debug log to \"%s\".\n",
	    pcFileName);
    return;
  }

  log_destroy(&pLogDebug);
  pLogDebug= pLogTmp;
}


/////////////////////////////////////////////////////////////////////
//
// API
//
/////////////////////////////////////////////////////////////////////

// -----[ libcbgp_exec_cmd ]-----------------------------------------
/**
 *
 */
CBGP_EXP_DECL
int libcbgp_exec_cmd(const char * cmd)
{
  return cli_execute_line(cli_get(), cmd);
}

// -----[ libcbgp_exec_file ]----------------------------------------
/**
 *
 */
CBGP_EXP_DECL
int libcbgp_exec_file(const char * file_name)
{
  FILE * pInCli= fopen(file_name, "r");

  if (pInCli == NULL) {
    LOG_ERR(LOG_LEVEL_SEVERE,
	    "Error: Unable to open script file \"%s\"\n", file_name);
    return -1;
  }
  
  if (libcbgp_exec_stream(pInCli) != CLI_SUCCESS) {
    fclose(pInCli);
    return -1;
  }

  fclose(pInCli);
  return 0;
}

// -----[ libcbgp_exec_stream ]--------------------------------------
/**
 *
 */
CBGP_EXP_DECL
int libcbgp_exec_stream(FILE * stream)
{
  return cli_execute_file(cli_get(), stream);
}

// -----[ libcbgp_interactive ]--------------------------------------
/**
 *
 */
CBGP_EXP_DECL
int libcbgp_interactive()
{
  int result= CLI_SUCCESS;
  char * line;

  libcbgp_banner();
  fprintf(stdout, "cbgp> init.\n");

  _rl_init();

  while (1) {
    /* Get user-input */
    line= rl_gets();

    /* EOF has been catched (Ctrl-D), exit */
    if (line == NULL) {
      fprintf(stdout, "\n");
      break;
    }

    /* Execute command */
    result= libcbgp_exec_cmd(line);

    if (result == CLI_SUCCESS_TERMINATE)
      break;

  }

  _rl_destroy();

  fprintf(stdout, "cbgp> done.\n");

  return result;
}


/////////////////////////////////////////////////////////////////////
//
// Initialization and configuration of the library (use with care).
//
/////////////////////////////////////////////////////////////////////

// -----[ libcbgp_init2 ]--------------------------------------------
/**
 * Initialize the C-BGP library, but not the GDS library. This
 * function is used by programs or libraries that run multiple C-BGP
 * C-BGP sessions and manage the GDS library by themselve (see the
 * JNI library for an example).
 *
 * NOTE: You should normaly not use this function. Consider using the
 * 'libcbgp_init' function instead.
 */
CBGP_EXP_DECL
void libcbgp_init2()
{
  // Initialize log.
  libcbgp_set_err_level(LOG_LEVEL_WARNING);
  libcbgp_set_debug_level(LOG_LEVEL_WARNING);

  // Hash init code commented in order to allow parameter setup
  // through he command-line/script (initialization is performed
  // just-in-time).
  //_comm_hash_init();
  //_path_hash_init();

  _network_create();
  _bgp_domain_init();
  _igp_domain_init();
  _ft_registry_init();
  _filter_path_regex_init();
  _route_map_init();
}

// -----[ libcbgp_done2 ]--------------------------------------------
/**
 * Free the resources allocated by the C-BGP library, but do not
 * terminate the GDS library. See the 'libcbgp_init2' function for
 * more details.
 *
 * NOTE: You should normaly not use this function. Consider using the
 * 'libcbgp_done' function instead. 
 */
CBGP_EXP_DECL
void libcbgp_done2()
{
  _cli_common_destroy();
  _message_destroy();
  _route_map_destroy();
  _filter_path_regex_destroy();
  _ft_registry_destroy();
  _bgp_domain_destroy();
  _igp_domain_destroy();
  _network_destroy();
  _mrtd_destroy();
  _path_hash_destroy();
  _comm_hash_destroy();
  _bgp_router_destroy();
  _aslevel_destroy();
  _path_destroy();
  _path_segment_destroy();
  _comm_destroy();
}




