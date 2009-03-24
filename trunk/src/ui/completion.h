// ==================================================================
// @(#)completion.h
//
// Provides functions to auto-complete commands/parameters in the CLI
// in interactive mode.
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 22/11/2002
// $Id: completion.h,v 1.2 2009-03-24 16:29:41 bqu Exp $
// ==================================================================

/**
 * \file
 * Provide data structures and function for handling auto-completion
 * in the command-line interface (CLI).
 */

#ifndef __UI_COMPLETION_H__
#define __UI_COMPLETION_H__

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ _rl_completion_init ]------------------------------------
  /**
   * \internal
   * Initialize the CLI auto-completion subsystem. This function
   * should be called by the library initialization function.
   */
  void _rl_completion_init();

  // -----[ _rl_completion_done ]------------------------------------
  /**
   * \internal
   * Finalize the CLI auto-completion subsystem. This function
   * should be called by the library finalization function.
   */
  void _rl_completion_done();

#ifdef __cplusplus
}
#endif

#endif /* __UI_COMPLETION_H__ */
