// ==================================================================
// @(#)help.h
//
// Provides functions to obtain help from the CLI in interactive mode.
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 22/11/2002
// $Id: help.h,v 1.2 2008-04-11 11:03:07 bqu Exp $
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
