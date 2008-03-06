// ==================================================================
// @(#)cisco.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 15/05/2007
// @lastdate 15/05/2007
// ==================================================================

#ifndef __BGP_CISCO_H__
#define __BGP_CISCO_H__

#include <stdio.h>

#include <libgds/log.h>

// ----- Error codes -----
#define CISCO_SUCCESS          0
#define CISCO_ERROR_UNEXPECTED -1
#define CISCO_ERROR_FILE_OPEN  -2
#define CISCO_ERROR_NUM_FIELDS -3

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ cisco_perror ]-------------------------------------------
  void cisco_perror(SLogStream * pStream, int iErrorCode);
  // -----[ cisco_parser ]-------------------------------------------
  int cisco_parser(FILE * pStream);
  // -----[ cisco_load ]---------------------------------------------
  int cisco_load(const char * pcFileName);

#ifdef __cplusplus
}
#endif

#endif /* __BGP_CISCO_H__ */