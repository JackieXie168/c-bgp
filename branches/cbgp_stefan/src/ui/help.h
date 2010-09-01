// ==================================================================
// @(#)help.h
//
// Provides functions to obtain help from the CLI in interactive mode.
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 22/11/2002
// $Id: help.h,v 1.3 2009-03-24 16:29:41 bqu Exp $
// ==================================================================

/**
 * \file
 * Provide functions to display help about CLI commands.
 */

#ifndef __UI_HELP_H__
#define __UI_HELP_H__

#include <libgds/cli.h>

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ cli_help ]-----------------------------------------------
  /**
   * Display CLI command help.
   *
   * This function provides help for the command passed in the given
   * string.
   *
   * \param cmd is the name of the command for which help is
   *   requested.
   */
  void cli_help(const char * cmd);

  // -----[ cli_help_file ]------------------------------------------
  /**
   * Return the path of an help file for a command.
   *
   * \param cmd is the name of the command for which help is
   *   requested.
   * \param buf is a buffer where the file name will be copied.
   * \param buf_len is the buffer size. The function will write at
   *   most buf_len-1 characters + the terminating zero.
   * \retval an error code.
   */
  int cli_help_file(const char * cmd, char * buf, size_t buf_len);

  // -----[ _help_init ]---------------------------------------------
  /**
   * \internal
   * Initialize the CLI help subsystem. This function must be called
   * by the library initialization function.
   */
  void _help_init();

  // -----[ _help_destroy ]------------------------------------------
  /**
   * \internal
   * Finalize the CLI help subsystem. This function must be called by
   * the library finalization function.
   */
  void _help_destroy();

#ifdef __cplusplus
}
#endif

#endif /* __UI_HELP_H__ */
