// ==================================================================
// @(#)api.h
//
// Application Programming Interface for the C-BGP library (libcsim).
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 25/10/2006
// $Id: api.h,v 1.8 2008-06-12 11:18:39 bqu Exp $
// ==================================================================

#ifndef __CBGP_API_H__
#define __CBGP_API_H__

#include <libgds/libgds-config.h>
#include <libgds/log.h>

#ifdef CYGWIN
# define API_EXPORT __declspec(dllexport)
#else
# define EXPORT
#endif

#ifdef __cplusplus
extern "C" {
#endif

  /*/////////////////////////////////////////////////////////////////
  //
  // Initialization and configuration of the library.
  //
  /////////////////////////////////////////////////////////////////*/

  // -----[ libcbgp_init ]-------------------------------------------
  EXPORT void libcbgp_init();
  // -----[ libcbgp_done ]-------------------------------------------
  EXPORT void libcbgp_done();
  // -----[ libcbgp_banner ]-----------------------------------------
  EXPORT void libcbgp_banner();
  // -----[ libcbgp_set_debug_callback ]-----------------------------
  EXPORT void libcbgp_set_debug_callback(FLogStreamCallback fCallback,
				  void * pContext);
  // -----[ libcbgp_set_err_callback ]-------------------------------
  EXPORT void libcbgp_set_err_callback(FLogStreamCallback fCallback,
				void * pContext);
  // -----[ libcbgp_set_out_callback ]-------------------------------
  EXPORT void libcbgp_set_out_callback(FLogStreamCallback fCallback,
				void * pContext);
  // -----[ libcbgp_set_debug_file ]---------------------------------
  EXPORT void libcbgp_set_debug_file(char * pcFileName);
  // -----[ libcbgp_set_debug_level ]--------------------------------
  EXPORT void libcbgp_set_debug_level(ELogLevel eLevel);
  // -----[ libcbgp_set_err_level ]----------------------------------
  EXPORT void libcbgp_set_err_level(ELogLevel eLevel);
  

  /*/////////////////////////////////////////////////////////////////
  //
  // API
  //
  /////////////////////////////////////////////////////////////////*/

  // -----[ libcbgp_exec_cmd ]---------------------------------------
  EXPORT int libcbgp_exec_cmd(const char * cmd);
  // -----[ libcbgp_exec_file ]--------------------------------------
  EXPORT int libcbgp_exec_file(const char * file_name);
  // -----[ libcbgp_exec_stream ]------------------------------------
  EXPORT int libcbgp_exec_stream(FILE * stream);
  // -----[ libcbgp_interactive ]------------------------------------
  EXPORT int libcbgp_interactive();


  
  /*/////////////////////////////////////////////////////////////////
  //
  // Other functions (use with care)
  //
  /////////////////////////////////////////////////////////////////*/

  // -----[ libcbgp_init2 ]------------------------------------------
  EXPORT void libcbgp_init2();
  // -----[ libcbgp_done2 ]------------------------------------------
  EXPORT void libcbgp_done2();


#ifdef __cplusplus
}
#endif

#endif /* __CBGP_API_H__ */
