// ==================================================================
// @(#)common.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 15/07/2003
// @lastdate 16/01/2007
// ==================================================================

#ifndef __CLI_COMMON_H__
#define __CLI_COMMON_H__

#include <libgds/cli.h>
#include <net/prefix.h>

#ifdef __cplusplus
extern "C" {
#endif

  // ----- cli_get --------------------------------------------------
  SCli * cli_get();
  
  // ----- cli_common_check_addr ------------------------------------
  int cli_common_check_addr(char * pcValue);
  // ----- cli_common_check_prefix ----------------------------------
  int cli_common_check_prefix(char * pcValue);
  // ----- cli_common_check_uint ------------------------------------
  int cli_common_check_uint(char * pcValue);
  
  // ----- _cli_destroy ---------------------------------------------
  void _cli_common_destroy();
  
#ifdef __cplusplus
}
#endif

#endif /* __CLI_COMMON_H__ */
