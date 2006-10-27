// ==================================================================
// @(#)api.h
//
// Application Programming Interface for the C-BGP library (libcsim).
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 25/10/2006
// @lastdate 27/10/2006
// ==================================================================

#ifndef __CBGP_API_H__
#define __CBGP_API_H__

#include <libgds/libgds-config.h>
#include <libgds/log.h>


/////////////////////////////////////////////////////////////////////
//
// Initialization and configuration of the library.
//
/////////////////////////////////////////////////////////////////////

// -----[ libcbgp_init ]---------------------------------------------
extern void libcbgp_init();
// -----[ libcbgp_done ]---------------------------------------------
extern void libcbgp_done();
// -----[ libcbgp_set_err_callback ]---------------------------------
extern void libcbgp_set_err_callback(FLogStreamCallback fCallback,
				     void * pContext);
// -----[ libcbgp_set_out_callback ]---------------------------------
extern void libcbgp_set_out_callback(FLogStreamCallback fCallback,
				     void * pContext);


/////////////////////////////////////////////////////////////////////
//
// API
//
/////////////////////////////////////////////////////////////////////

// -----[ libcbgp_exec_cmd ]-----------------------------------------
extern int libcbgp_exec_cmd(const char * pcCmd);
// -----[ libcbgp_exec_file ]----------------------------------------
extern int libcbgp_exec_file(const char * pcFileName);


#endif /* __CBGP_API_H__ */
