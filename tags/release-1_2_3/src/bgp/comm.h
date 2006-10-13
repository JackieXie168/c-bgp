// ==================================================================
// @(#)comm.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 23/05/2003
// @lastdate 03/03/2006
// ==================================================================

#ifndef __COMM_H__
#define __COMM_H__

#include <libgds/log.h>

#include <bgp/comm_t.h>

// ----- comm_create ------------------------------------------------
extern SCommunities * comm_create();
// ----- comm_destroy -----------------------------------------------
extern void comm_destroy(SCommunities ** ppComm);
// ----[ comm_copy ]-------------------------------------------------
extern SCommunities * comm_copy(SCommunities * pComm);
// ----- comm_add ---------------------------------------------------
extern int comm_add(SCommunities ** ppComms, comm_t tComm);
// ----- comm_remove ------------------------------------------------
extern void comm_remove(SCommunities ** ppComms, comm_t tComm);
// ----- comm_contains ----------------------------------------------
extern int comm_contains(SCommunities * pComms, comm_t tComm);
// ----- comm_equals ------------------------------------------------
extern int comm_equals(SCommunities * pComms1,
		       SCommunities * pComms2);
// ----- comm_from_string -------------------------------------------
extern int comm_from_string(char * pcComm, comm_t * pComm);
// ----- comm_dump2 -------------------------------------------------
extern void comm_dump2(SLogStream * pStream, comm_t tComm,
		       int iText);
// ----- comm_dump --------------------------------------------------
extern void comm_dump(SLogStream * pStream, SCommunities * pComms,
		      int iText);
// -----[ comm_to_string ]-------------------------------------------
extern int comm_to_string(SCommunities * pComms, char * pBuffer,
			  size_t tBufferSize);

#endif
