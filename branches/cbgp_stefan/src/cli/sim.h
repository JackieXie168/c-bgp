// ==================================================================
// @(#)sim.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 24/07/2003
// $Id: sim.h,v 1.3 2009-03-24 15:58:43 bqu Exp $
// ==================================================================

#ifndef __CLI_SIM_H__
#define __CLI_SIM_H__

#include <libgds/cli.h>

#ifdef __cplusplus
extern "C" {
#endif

  // ----- cli_register_sim -----------------------------------------
  void cli_register_sim(cli_cmd_t * parent);

#ifdef __cplusplus
}
#endif

#endif /* __CLI_SIM_H__ */
