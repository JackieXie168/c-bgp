// ==================================================================
// @(#)api.h
//
// Application Programming Interface for the C-BGP library (libcsim).
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 25/10/2006
// $Id: api.h,v 1.5 2008-04-07 09:21:21 bqu Exp $
// ==================================================================

#ifndef __CBGP_API_H__
#define __CBGP_API_H__

#include <libgds/libgds-config.h>
#include <libgds/log.h>

#ifdef __cplusplus
extern "C" {
#endif

  /*/////////////////////////////////////////////////////////////////
  //
  // Initialization and configuration of the library.
  //
  /////////////////////////////////////////////////////////////////*/

  // -----[ libcbgp_init ]-------------------------------------------
  void libcbgp_init();
  // -----[ libcbgp_done ]-------------------------------------------
  void libcbgp_done();
  // -----[ libcbgp_banner ]-----------------------------------------
  void libcbgp_banner();
  // -----[ libcbgp_set_debug_callback ]-----------------------------
  void libcbgp_set_debug_callback(FLogStreamCallback fCallback,
				  void * pContext);
  // -----[ libcbgp_set_err_callback ]-------------------------------
  void libcbgp_set_err_callback(FLogStreamCallback fCallback,
				void * pContext);
  // -----[ libcbgp_set_out_callback ]-------------------------------
  void libcbgp_set_out_callback(FLogStreamCallback fCallback,
				void * pContext);
  // -----[ libcbgp_set_debug_file ]---------------------------------
  void libcbgp_set_debug_file(char * pcFileName);
  // -----[ libcbgp_set_debug_level ]--------------------------------
  void libcbgp_set_debug_level(ELogLevel eLevel);
  // -----[ libcbgp_set_err_level ]----------------------------------
  void libcbgp_set_err_level(ELogLevel eLevel);
  

  /*/////////////////////////////////////////////////////////////////
  //
  // API
  //
  /////////////////////////////////////////////////////////////////*/

  // -----[ libcbgp_exec_cmd ]---------------------------------------
  int libcbgp_exec_cmd(const char * pcCmd);
  // -----[ libcbgp_exec_file ]--------------------------------------
  int libcbgp_exec_file(const char * pcFileName);
  // -----[ libcbgp_exec_stream ]------------------------------------
  int libcbgp_exec_stream(FILE * pStream);

  
  /*/////////////////////////////////////////////////////////////////
  //
  // Other functions (use with care)
  //
  /////////////////////////////////////////////////////////////////*/

  // -----[ libcbgp_init2 ]------------------------------------------
  void libcbgp_init2();
  // -----[ libcbgp_done2 ]------------------------------------------
  void libcbgp_done2();


#ifdef __cplusplus
}
#endif

#endif /* __CBGP_API_H__ */
