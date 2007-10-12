// ==================================================================
// @(#)comm.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 23/05/2003
// @lastdate 20/07/2007
// ==================================================================

#ifndef __BGP_COMM_H__
#define __BGP_COMM_H__

#include <libgds/log.h>

#include <bgp/comm_t.h>

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ comm_create ]--------------------------------------------
  SCommunities * comm_create();
  // -----[ comm_destroy ]-------------------------------------------
  void comm_destroy(SCommunities ** ppComm);
  // -----[ comm_copy ]----------------------------------------------
  SCommunities * comm_copy(SCommunities * pComm);
  // ----- comm_add -------------------------------------------------
  int comm_add(SCommunities ** ppComms, comm_t tComm);
  // ----- comm_remove ----------------------------------------------
  void comm_remove(SCommunities ** ppComms, comm_t tComm);
  // ----- comm_contains --------------------------------------------
  int comm_contains(SCommunities * pComms, comm_t tComm);
  // ----- comm_equals ----------------------------------------------
  int comm_equals(SCommunities * pComms1, SCommunities * pComms2);

  // -----[ comm_value_from_string ]---------------------------------
  int comm_value_from_string(char * pcComm, comm_t * pComm);
  // -----[ comm_value_dump ]----------------------------------------
  void comm_value_dump(SLogStream * pStream, comm_t tComm, int iText);

  // ----- comm_dump ------------------------------------------------
  void comm_dump(SLogStream * pStream, SCommunities * pComms,
		 int iText);
  // -----[ comm_to_string ]-----------------------------------------
  int comm_to_string(SCommunities * pComms, char * pBuffer,
		     size_t tBufferSize);
  // -----[ comm_from_string ]---------------------------------------
  SCommunities * comm_from_string(const char * pcCommunities);

  // -----[ comm_cmp ]-----------------------------------------------
  int comm_cmp(SCommunities * pComm1, SCommunities * pComm2);

  // -----[ _comm_destroy ]------------------------------------------
  void _comm_destroy();

#ifdef __cplusplus
}
#endif

#endif /* __BGP_COMM_H__ */
