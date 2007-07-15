// ==================================================================
// @(#)bgp.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 15/07/2003
// @lastdate 16/01/2007
// ==================================================================

#ifndef __CLI_BGP_H__
#define __CLI_BGP_H__

#include <libgds/cli.h>

#ifdef __cplusplus
extern "C" {
#endif

  // ----- cli_register_bgp -----------------------------------------
  int cli_register_bgp(SCli * pCli);

#ifdef __cplusplus
}
#endif

#endif /* __CLI_BGP_H__ */
