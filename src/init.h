// ==================================================================
// @(#)init.h
//
// Initialize the C-BGP library.
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 25/10/2006
// @lastdate 26/10/2006
// ==================================================================

#ifndef __LIBCSIM_INIT_H__
#define __LIBCSIM_INIT_H__

#include <libgds/libgds-config.h>
#include <libgds/log.h>

// -----[ libcsim_init ]-------------------------------------------
extern void libcsim_init();
// -----[ libcsim_done ]-------------------------------------------
extern void libcsim_done();
// -----[ libcsim_send ]-------------------------------------------
extern int libcsim_send(const char * pcLine);
// -----[ libcsim_exec ]-------------------------------------------
extern int libcsim_exec(const char * pcLine);

// -----[ libcsim_set_err_callback ]---------------------------------
extern void libcsim_set_err_callback(FLogStreamCallback fCallback,
				     void * pContext);
// -----[ libcsim_set_out_callback ]---------------------------------
extern void libcsim_set_out_callback(FLogStreamCallback fCallback,
				     void * pContext);

#endif /* __LIBCSIM_INIT_H__ */
