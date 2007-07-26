// ==================================================================
// @(#)main.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 28/01/2005
// @lastdate 12/10/2005
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <assert.h>
#include <libgds/gds.h>

#include <bgp/as.h>
#include <bgp/domain.h>
#include <bgp/filter_registry.h>
#include <bgp/mrtd.h>
#include <bgp/path_hash.h>
#include <bgp/peer_t.h>
#include <bgp/predicate_parser.h>
#include <bgp/qos.h>
#include <bgp/rexford.h>
#include <bgp/route_map.h>
#include <bgp/route_reflector.h>
#include <cli/common.h>
#include <libgds/fifo.h>
#include <libgds/memory.h>
#include <net/igp_domain.h>
#include <net/network.h>
#include <net/net_path.h>
#include <sim/simulator.h>
#include <libgds/str_util.h>

/////////////////////////////////////////////////////////////////////
// CHECKING MACROS
/////////////////////////////////////////////////////////////////////

#define MSG_CHECKING(MSG) \
  printf("TESTING \033[37;1m%s\033[0m", MSG)
#define MSG_RESULT_SUCCESS() \
  printf("\033[70G[\033[32;1mSUCCESS\033[0m]\n")
#define MSG_RESULT_FAIL() \
  printf("\033[70G[\033[31;1mFAIL\033[0m]\n")
#define ASSERT_RETURN(TEST, INFO) \
  if (!(TEST)) { \
    MSG_RESULT_FAIL(); \
    printf("  Reason: [%s]\n", INFO); \
    return -1; \
  }
#define TEST_RETURN(TEST, INFO) \
  if (!(TEST)) { \
    MSG_RESULT_FAIL(); \
    printf("  Reason: [%s]\n", INFO); \
    return -1; \
  } \
  MSG_RESULT_SUCCESS();

void _main_init() __attribute((constructor));
void _main_done() __attribute((destructor));

void _main_init()
{
  gds_init(0);
  //gds_init(GDS_OPTION_MEMORY_DEBUG);

  _path_hash_init();
  _network_create();
  _bgp_domain_init();
  _igp_domain_init();
  _ft_registry_init();
  _filter_path_regex_init();
  _route_map_init();
  _rexford_init();
}

void _main_done()
{
  _cli_common_destroy();
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

  gds_destroy();
}

// -----[ build_config_rt1_vt1 ]-------------------------------------
/**
 * Build a test configuration composed of two nodes RT1 and VT1. Node
 * RT1 is a BGP router while VT1 is a virtual BGP router.
 *
 * RT1 has IP address 1.0.0.0
 * VT1 has IP address 2.0.0.0
 * RT1 has an eBGP session with VT1
 *
 * Return value:
 *   0    success
 *   -1   failure
 */
/*int build_config_rt1_vt1()
{
  SNetNode * pRT1= node_create(IPV4_TO_INT(1,0,0,0));
  SNetNode * pVT1= node_create(IPV4_TO_INT(2,0,0,0));
  SNetLink * pLink;
    
  assert(pRT1 != NULL);
  assert(pRT2 != NULL);
  assert(!network_add_node(pRT1));
  assert(!network_add_node(pVT1));

  assert(node_add_link_to_router(pRT1, pVT1, 100, 1));

  assert(!node_register_protocol(pRT1, NET_PROTOCOL_BGP, pRT1, NULL,
				node_handle_event));
				}*/

/////////////////////////////////////////////////////////////////////
// DECISION PROCESS TEST
/////////////////////////////////////////////////////////////////////
int check_decision_process()
{
  MSG_CHECKING("* Decision process");
  MSG_RESULT_FAIL();

  return -1;
}

/////////////////////////////////////////////////////////////////////
// VARIOUS SCRIPTS
////////////////////////////////////////////////////////////////////
int check_script(char * pcFileName)
{
  FILE * pInCli;
  int iResult= CLI_ERROR_UNEXPECTED;

  MSG_CHECKING(pcFileName);
  log_set_level(pMainLog, LOG_LEVEL_FATAL);
  log_set_stream(pMainLog, stderr);

  simulator_init();
  pInCli= fopen(pcFileName, "r");
  if (pInCli != NULL) {
    iResult= cli_execute_file(cli_get(), pInCli);
    fclose(pInCli);
  }
  simulator_done();
  if (iResult != CLI_SUCCESS)
    return -1;
  return 0;
}

int check_scripts()
{
  TEST_RETURN(check_script("valid/valid-igp-change.cli"),
	      "script execution failed");
  TEST_RETURN(check_script("valid/valid-static-route.cli"),
	      "script execution failed");
  return 0;
}

/////////////////////////////////////////////////////////////////////
// MAIN PART
/////////////////////////////////////////////////////////////////////

// -----[ main ]-----------------------------------------------------
int main(int argc, char * argv[])
{
  //check_decision_process();
  check_scripts();

  return 0;
}
