// ==================================================================
// @(#)rl.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 23/02/2004
// @lastdate 31/03/2006
// ==================================================================

#ifndef __RL_H__
#define __RL_H__

#include <string.h>

// ---- rl_gets -----------------------------------------------------
extern char * rl_gets(char * pcPrompt);

#undef _FILENAME_COMPLETION_FUNCTION
#ifdef HAVE_LIBREADLINE
# include <readline/readline.h>
# ifdef HAVE_RL_FILENAME_COMPLETION_FUNCTION
#  define _FILENAME_COMPLETION_FUNCTION rl_filename_completion_function
# else
#  ifdef HAVE_FILENAME_COMPLETION_FUNCTION
char * rl_filename_completion_function(const char * pcText, int iState)
{
  char acTextNotConst[256];
  strncpy(acTextNotConst, pcText, sizeof(acTextNotConst));
  acTextNotConst[sizeof(acTextNotConst)-1]= '\0';
  return filename_completion_function(acTextNotConst, iState);
}
#   define _FILENAME_COMPLETION_FUNCTION rl_filename_completion_function
#  endif
# endif
#endif

// ----- _rl_init ---------------------------------------------------
extern void _rl_init();
// ----- _rl_destroy ------------------------------------------------
extern void _rl_destroy();

#endif
