// ==================================================================
// @(#)rl.h
//
// Provides an interactive CLI, based on GNU readline. If GNU
// readline isn't available, provides a basic replacement CLI.
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 23/02/2004
// @lastdate 22/06/2007
// ==================================================================

#ifndef __UI_RL_H__
#define __UI_RL_H__

#include <string.h>

// -----[ rl_on_new_line ]-----
/* This functions is used to enumerate files during the command-line
 * auto-completion.
 */
#undef _FILENAME_COMPLETION_FUNCTION
#if defined(HAVE_LIBREADLINE) && defined(HAVE_READLINE_READLINE_H)
# include <readline/readline.h>
# ifdef HAVE_RL_FILENAME_COMPLETION_FUNCTION
#  define _FILENAME_COMPLETION_FUNCTION rl_filename_completion_function
# endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

  // ----[ rl_gets ]-------------------------------------------------
  char * rl_gets();

  // -----[ _rl_init ]-----------------------------------------------
  void _rl_init();
  // -----[ _rl_destroy ]--------------------------------------------
  void _rl_destroy();

#ifdef __cplusplus
}
#endif

#endif /* __UI_RL_H__ */
