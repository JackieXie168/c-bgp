// ==================================================================
// @(#)api.c
//
// Application Programming Interface for the C-BGP library (libcsim).
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 25/10/2006
// @lastdate 27/10/2006
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#ifdef HAVE_LIBREADLINE
//# include <readline/readline.h>
#endif

#include <api.h>

#include <libgds/gds.h>

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
#include <net/igp_domain.h>


/////////////////////////////////////////////////////////////////////
//
// Initialization and configuration of the library.
//
/////////////////////////////////////////////////////////////////////

// -----[ libcbgp_init ]---------------------------------------------
/**
 * Initialize the C-BGP library.
 */
void libcbgp_init()
{
#ifdef __MEMORY_DEBUG__
  gds_init(GDS_OPTION_MEMORY_DEBUG);
#else
  gds_init(0);
#endif

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
  simulator_init();
}

// -----[ libcbgp_done ]---------------------------------------------
/**
 * Free the resources allocated by the C-BGP library.
 */
void libcbgp_done()
{
  simulator_done();
  _cli_common_destroy();
#ifdef HAVE_LIBREADLINE
  //  _rl_destroy();
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
  _bgp_router_destroy();

  gds_destroy();
}

// -----[ libcbgp_set_err_callback ]---------------------------------
/**
 *
 */
void libcbgp_set_err_callback(FLogStreamCallback fCallback,
			      void * pContext)
{
  log_destroy(&pLogErr);
  pLogErr= log_create_callback(fCallback, pContext);
}

// -----[ libcbgp_set_out_callback ]---------------------------------
/**
 *
 */
void libcbgp_set_out_callback(FLogStreamCallback fCallback,
			      void * pContext)
{
  log_destroy(&pLogOut);
  pLogOut= log_create_callback(fCallback, pContext);
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
int libcbgp_exec_cmd(const char * pcCmd)
{
  return cli_execute_line(cli_get(), pcCmd);
}

// -----[ libcbgp_exec_file ]----------------------------------------
/**
 *
 */
int libcbgp_exec_file(const char * pcFileName)
{
  FILE * pInCli= fopen(pcFileName, "r");

  if (pInCli == NULL) {
    LOG_ERR(LOG_LEVEL_SEVERE,
	    "Error: Unable to open script file \"%s\"\n", pcFileName);
    return -1;
  }
  
  if (cli_execute_file(cli_get(), pInCli) != CLI_SUCCESS) {
    fclose(pInCli);
    return -1;
  }

  fclose(pInCli);
  return 0;
}

