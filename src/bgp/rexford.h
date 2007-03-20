// ==================================================================
// @(#)rexford.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 28/07/2003
// @lastdate 15/01/2007
// ==================================================================

#ifndef __BGP_REXFORD_H__
#define __BGP_REXFORD_H__

#include <libgds/log.h>

#include <net/prefix.h>
#include <net/network.h>
#include <stdio.h>

// ----- AS relationships and policies -----
#define REXFORD_REL_PEER_PEER 0
#define REXFORD_REL_PROV_CUST 1

// ----- Error codes ----
#define REXFORD_SUCCESS 0
#define REXFORD_ERROR_UNEXPECTED       -1
#define REXFORD_ERROR_OPEN             -2
#define REXFORD_ERROR_NUM_PARAMS       -3
#define REXFORD_ERROR_INVALID_ASNUM    -4
#define REXFORD_ERROR_INVALID_RELATION -5
#define REXFORD_ERROR_INVALID_DELAY    -6
#define REXFORD_ERROR_DUPLICATE_LINK   -7

// ----- Marking communities -----
#define COMM_PROV 1
#define COMM_PEER 10

// ----- Local preferences
#define PREF_PROV 60
#define PREF_PEER 80
#define PREF_CUST 100

#ifdef __cplusplus
extern "C" {
#endif

  // ----- rexford_get_as -------------------------------------------
  SBGPRouter * rexford_get_as(uint16_t uASNum);
  // ----- rexford_load ---------------------------------------------
  int rexford_load(char * pcFileName);
  // ----- rexford_setup_policies -----------------------------------
  void rexford_setup_policies();
  // ----- rexford_run ----------------------------------------------
  int rexford_run();
  // ----- rexford_record_route -------------------------------------
  int rexford_record_route(SLogStream * pStream, char * pcFileName,
				  SPrefix sPrefix);
  // ----- rexford_record_route_bm ----------------------------------
  int rexford_record_route_bm(SLogStream * pStream, char * pcFileName,
				     SPrefix sPrefix, uint8_t uBound);
  // ----- _rexford_init --------------------------------------------
  void _rexford_init();
  // ----- _rexford_destroy -----------------------------------------
  void _rexford_destroy();

#ifdef __cplusplus
}
#endif

#endif

