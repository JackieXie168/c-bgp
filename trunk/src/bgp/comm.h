// ==================================================================
// @(#)comm.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 23/05/2003
// @lastdate 14/10/2005
// ==================================================================

#ifndef __COMM_H__
#define __COMM_H__

#include <stdio.h>

#include <bgp/comm_t.h>

// ----- comm_create ------------------------------------------------
extern SCommunities * comm_create();
// ----- comm_destroy -----------------------------------------------
extern void comm_destroy(SCommunities ** ppCommunities);
// ----[ comm_copy ]-------------------------------------------------
extern SCommunities * comm_copy(SCommunities * pCommunities);
// ----- comm_add ---------------------------------------------------
extern int comm_add(SCommunities * pComm, comm_t uComm);
// ----- comm_remove ------------------------------------------------
extern void comm_remove(SCommunities * pCommunities,
			comm_t uCommunity);
// ----- comm_contains ----------------------------------------------
extern int comm_contains(SCommunities * pCommunities,
			 comm_t uCommunity);
// ----- comm_equals ------------------------------------------------
extern int comm_equals(SCommunities * pCommunities1,
		       SCommunities * pCommunities2);
// ----- comm_from_string -------------------------------------------
extern int comm_from_string(char * pcComm, comm_t * pCommunity);
// ----- comm_dump2 -------------------------------------------------
extern void comm_dump2(FILE * pStream, comm_t tCommunity,
		       int iText);
// ----- comm_dump --------------------------------------------------
extern void comm_dump(FILE * pStream, SCommunities * pCommunities,
		      int iText);
// -----[ comm_to_string ]-------------------------------------------
extern int comm_to_string(SCommunities * pCommunities, char * pBuffer,
			  size_t tBufferSize);

#endif
