// ==================================================================
// @(#)main-dump.c
//
// Main source file for cbgp-dump application.
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @lastdate 21/05/2007
// @date 21/05/07
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libgds/log.h>
#include <api.h>
#include <bgp/as.h>
#include <bgp/filter.h>
#include <bgp/peer.h>
#include <bgp/predicate_parser.h>
#include <bgp/route.h>
#include <bgp/route-input.h>

#define OPTION_HELP       0
#define OPTION_IN_FORMAT  1
#define OPTION_OUT_FORMAT 2
#define OPTION_OUT_SPEC   3
#define OPTION_FILTER     4

static struct option longopts[]= {
  {"filter", required_argument, NULL, OPTION_FILTER},
  {"help", no_argument, NULL, OPTION_HELP},
  {"in-fmt", required_argument, NULL, OPTION_IN_FORMAT},
  {"out-fmt", required_argument, NULL, OPTION_OUT_FORMAT},
  {"out-spec", required_argument, NULL, OPTION_OUT_SPEC},
};

typedef struct {
  unsigned int uRoutesOk;
  unsigned int uRoutesIgnored;
  unsigned int uRoutesFiltered;
  char * pcOutSpec;
  SFilterMatcher * pMatcher;
} SDumperContext;
static SDumperContext sContext= { .uRoutesOk= 0,
				  .uRoutesIgnored= 0,
				  .uRoutesFiltered= 0,
				  .pcOutSpec= NULL,
				  .pMatcher= NULL,
};

// -----[ usage ]----------------------------------------------------
void usage()
{
  printf("C-BGP route dumper %s\n", PACKAGE_VERSION);
  printf("Copyright (C) 2007, Bruno Quoitin\n");
  printf("IP Networking Lab, CSE Dept, UCL, Belgium\n");
  printf("\nUsage: cbgp-dump [OPTION]... [FILE]...\n\n");
  printf("  --filter=FILTER     specifies a route filter.\n");
  printf("  --help              prints this message.\n");
  printf("  --in-fmt=FORMAT     specifies the input file format. The default format is\n");
  printf("                      MRT ASCII. Available file formats are:\n");
  printf("                        (cisco)      CISCO's show ip bgp format\n");
  printf("                        (mrt-ascii)  MRT ASCII format\n");
  printf("                        (mrt-binary) MRT binary\n");
  printf("  --out-fmt=FORMAT    specifies the output format. The default format is\n");
  printf("                      CISCO's show ip bgp. Available file formats are:\n");
  printf("                        (cisco)      CISCO's show ip bgp format\n");
  printf("                        (mrt-ascii)  MRT ASCII format\n");
  printf("                        (custom)     custom format (see --out-spec)\n");
  printf("  --out-spec=SPEC     specifies the format string for the custom output format.\n");
  printf("\n");
}

// -----[ _handler ]-------------------------------------------------
static int _handler(int iStatus, SRoute * pRoute, net_addr_t tPeerAddr,
		    unsigned int uPeerAS, void * pContext)
{
  SDumperContext * pCtx= (SDumperContext *) pContext;
  SBGPPeer sPeer;

  // Maintain statistics for discarded routes
  if (iStatus != BGP_ROUTES_INPUT_STATUS_OK) {
    switch (iStatus) {
    case BGP_ROUTES_INPUT_STATUS_IGNORED:
      pCtx->uRoutesIgnored++;
      break;
    case BGP_ROUTES_INPUT_STATUS_FILTERED:
      pCtx->uRoutesFiltered++;
      break;
    }
    return BGP_ROUTES_INPUT_SUCCESS;
  }

  // Filter route (if filter specified)
  if (pCtx->pMatcher != NULL) {
    if (filter_matcher_apply(pCtx->pMatcher, NULL, pRoute) != 1) {
      pCtx->uRoutesFiltered++;
      route_destroy(&pRoute);
      return BGP_ROUTES_INPUT_SUCCESS;
    }
  }

  // Create fake BGP peer
  sPeer.tAddr= tPeerAddr;
  sPeer.uRemoteAS= uPeerAS;
  pRoute->pPeer= &sPeer;

  // Display route
  //log_printf(pLogOut, "TABLE_DUMP|0|");
  route_dump(pLogOut, pRoute);
  log_flush(pLogOut);
  log_printf(pLogOut, "\n");
  pCtx->uRoutesOk++;
  route_destroy(&pRoute);
  return BGP_ROUTES_INPUT_SUCCESS;
}

// -----[ _main_init ]-----------------------------------------------
static void _main_init() __attribute((constructor));
static void _main_init()
{
  libcbgp_init();
}

// -----[ _main_done ]-----------------------------------------------
static void _main_done() __attribute((destructor));
static void _main_done()
{
  if (sContext.pcOutSpec != NULL)
    free(sContext.pcOutSpec);
  if (sContext.pMatcher != NULL)
    filter_matcher_destroy(&sContext.pMatcher);

  libcbgp_done();
}

// -----[ main ]-----------------------------------------------------
int main(int argc, char * argv[])
{
  uint8_t uInFormat= BGP_ROUTES_INPUT_MRT_ASC;
  uint8_t uOutFormat= BGP_ROUTES_OUTPUT_CISCO;
  int iOption, iResult;
  char * pcFilter;

  // Parse options
  while ((iOption= getopt_long(argc, argv, "", longopts, NULL)) != -1) {
    switch (iOption) {
    case OPTION_FILTER:
      pcFilter= strdup(optarg);
      if (sContext.pMatcher != NULL) {
	fprintf(stderr, "Error: route filter already specified.\n");
	return EXIT_FAILURE;
      }
      if (predicate_parse(&pcFilter, &sContext.pMatcher) != 0) {
	fprintf(stderr, "Error: invalid route filter \"%s\"\n", optarg);
	return EXIT_FAILURE;
      }
      break;
    case OPTION_HELP:
      usage();
      return EXIT_SUCCESS;
    case OPTION_IN_FORMAT:
      if (bgp_routes_str2format(optarg, &uInFormat) != 0) {
	fprintf(stderr, "Error: invalid input format \"%s\".\n", optarg);
	return EXIT_FAILURE;
      }
      break;
    case OPTION_OUT_FORMAT:
      if (route_str2format(optarg, &uOutFormat) != 0) {
	fprintf(stderr, "Error: invalid output format \"%s\".\n", optarg);
	return EXIT_FAILURE;
      }
      break;
    case OPTION_OUT_SPEC:
      if (sContext.pcOutSpec != NULL) {
	fprintf(stderr, "Error: output spec already specified.\n");
	return EXIT_FAILURE;
      }
      sContext.pcOutSpec= strdup(optarg);
      break;
    default:
      usage();
      return EXIT_FAILURE;
    }
  }

  if (optind >= argc) {
    fprintf(stderr, "Error: no input file.\n");
    return EXIT_FAILURE;
  }

  BGP_OPTIONS_SHOW_MODE= uOutFormat;
  if (sContext.pcOutSpec != NULL)
    BGP_OPTIONS_SHOW_FORMAT= strdup(sContext.pcOutSpec);

  // Parse input file(s)
  while (optind < argc) {

    log_printf(pLogErr, "Reading from \"%s\"...\n", argv[optind]);

    iResult= bgp_routes_load(argv[optind], uInFormat, _handler, &sContext);
    if (iResult != 0) {
      LOG_ERR(LOG_LEVEL_SEVERE, "Error: an error (%d) occured while reading \"%s\"", iResult, argv[optind]);
      libcbgp_done();
      return EXIT_SUCCESS;
    }
    optind++;
  }

  // Print statistics
  log_printf(pLogErr, "\n");
  log_printf(pLogErr, "Summary:\n");
  log_printf(pLogErr, "  routes dumped  : %d\n", sContext.uRoutesOk);
  log_printf(pLogErr, "  routes ignored : %d\n", sContext.uRoutesIgnored);
  log_printf(pLogErr, "  routes filtered: %d\n", sContext.uRoutesFiltered);

  return EXIT_SUCCESS;
}
