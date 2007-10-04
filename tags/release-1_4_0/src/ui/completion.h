// ==================================================================
// @(#)completion.h
//
// Provides functions to auto-complete commands/parameters in the CLI
// in interactive mode.
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 22/11/2002
// @lastdate 22/06/2007
// ==================================================================

#ifndef __UI_COMPLETION_H__
#define __UI_COMPLETION_H__

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ _rl_completion_init ]------------------------------------
  void _rl_completion_init();
  // -----[ _rl_completion_done ]------------------------------------
  void _rl_completion_done();

#ifdef __cplusplus
}
#endif

#endif /* __UI_COMPLETION_H__ */
