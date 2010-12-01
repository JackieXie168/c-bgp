// ==================================================================
// @(#)tracer_cli.h
//
// @author Stefan Beauport (stefan.beauport@umons.Ac.be)


// ==================================================================

#ifndef __CLI_TRACER_H__
#define __CLI_TRACER_H__

#include <libgds/cli.h>

#ifdef __cplusplus
extern "C" {
#endif

  // ----- cli_register_sim -----------------------------------------
  void cli_register_tracer(cli_cmd_t * parent);

#ifdef __cplusplus
}
#endif

#endif /* __CLI_TRACER_H__ */
