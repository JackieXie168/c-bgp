// ==================================================================
// @(#)rexford.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 28/07/2003
// @lastdate 12/02/2005
// ==================================================================

#ifndef __BGP_REXFORD_H__
#define __BGP_REXFORD_H__

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

// ----- rexford_get_as ---------------------------------------------
extern SBGPRouter * rexford_get_as(uint16_t uASNum);
// ----- rexford_load -----------------------------------------------
extern int rexford_load(char * pcFileName, SNetwork * pNetwork);
// ----- rexford_setup_policies -------------------------------------
extern void rexford_setup_policies();
// ----- rexford_run ------------------------------------------------
extern int rexford_run();
// ----- rexford_record_route ---------------------------------------
extern int rexford_record_route(FILE * pStream, char * pcFileName,
				SPrefix sPrefix);
// ----- rexford_record_route_bm ------------------------------------
extern int rexford_record_route_bm(FILE * pStream, char * pcFileName,
				   SPrefix sPrefix, uint8_t uBound);
// ----- rexford_route_dp_rule --------------------------------------
/*
extern int rexford_route_dp_rule(FILE * pStream, SPrefix sPrefix);
*/

#endif

