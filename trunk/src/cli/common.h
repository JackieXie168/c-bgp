// ==================================================================
// @(#)common.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 15/07/2003
// @lastdate 17/05/2005
// ==================================================================

#ifndef __COMMON_H__
#define __COMMON_H__

#include <libgds/cli.h>
#include <net/prefix.h>

// ----- cli_get ----------------------------------------------------
extern SCli * cli_get();

// ----- cli_common_check_addr --------------------------------------
extern int cli_common_check_addr(char * pcValue);
// ----- cli_common_check_prefix ------------------------------------
extern int cli_common_check_prefix(char * pcValue);
// ----- cli_common_check_uint --------------------------------------
extern int cli_common_check_uint(char * pcValue);

// ----- _cli_destroy -----------------------------------------------
extern void _cli_common_destroy();

#endif
