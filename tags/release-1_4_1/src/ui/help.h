// ==================================================================
// @(#)help.h
//
// Provides functions to obtain help from the CLI in interactive mode.
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 22/11/2002
// @lastdate 25/06/2007
// ==================================================================

#ifndef __UI_HELP_H__
#define __UI_HELP_H__

#include <libgds/cli.h>

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ cli_help ]-----------------------------------------------
  void cli_help(SCli * pCli, char * pcLine, int iPos);

  // -----[ _rl_help_init ]--------------------------------------------
  void _rl_help_init();
  // -----[ _rl_help_done ]--------------------------------------------
  void _rl_help_done();

#ifdef __cplusplus
}
#endif

#endif /* __CLI_HELP_H__ */
