// ==================================================================
// @(#)api.h
//
// Application Programming Interface for the C-BGP library (libcsim).
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 25/10/2006
// $Id: api.h,v 1.11 2008-06-16 09:58:24 bqu Exp $
// ==================================================================

#ifndef __CBGP_API_H__
#define __CBGP_API_H__

#include <libgds/libgds-config.h>
#include <libgds/log.h>

#ifdef CYGWIN
# define CBGP_EXP_DECL __declspec(dllexport)
#else
# define CBGP_EXP_DECL
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
  CBGP_EXP_DECL void libcbgp_init();
  // -----[ libcbgp_done ]-------------------------------------------
  CBGP_EXP_DECL void libcbgp_done();
  // -----[ libcbgp_banner ]-----------------------------------------
  CBGP_EXP_DECL void libcbgp_banner();
  // -----[ libcbgp_set_debug_callback ]-----------------------------
  CBGP_EXP_DECL void libcbgp_set_debug_callback(FLogStreamCallback fCallback,
				  void * pContext);
  // -----[ libcbgp_set_err_callback ]-------------------------------
  CBGP_EXP_DECL void libcbgp_set_err_callback(FLogStreamCallback fCallback,
				void * pContext);
  // -----[ libcbgp_set_out_callback ]-------------------------------
  CBGP_EXP_DECL void libcbgp_set_out_callback(FLogStreamCallback fCallback,
				void * pContext);
  // -----[ libcbgp_set_debug_file ]---------------------------------
  CBGP_EXP_DECL void libcbgp_set_debug_file(char * pcFileName);
  // -----[ libcbgp_set_debug_level ]--------------------------------
  CBGP_EXP_DECL void libcbgp_set_debug_level(ELogLevel eLevel);
  // -----[ libcbgp_set_err_level ]----------------------------------
  CBGP_EXP_DECL void libcbgp_set_err_level(ELogLevel eLevel);
  

  /*/////////////////////////////////////////////////////////////////
  //
  // API
  //
  /////////////////////////////////////////////////////////////////*/

  // -----[ libcbgp_exec_cmd ]---------------------------------------
  CBGP_EXP_DECL int libcbgp_exec_cmd(const char * cmd);
  // -----[ libcbgp_exec_file ]--------------------------------------
  CBGP_EXP_DECL int libcbgp_exec_file(const char * file_name);
  // -----[ libcbgp_exec_stream ]------------------------------------
  CBGP_EXP_DECL int libcbgp_exec_stream(FILE * stream);
  // -----[ libcbgp_interactive ]------------------------------------
  CBGP_EXP_DECL int libcbgp_interactive();


  
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
