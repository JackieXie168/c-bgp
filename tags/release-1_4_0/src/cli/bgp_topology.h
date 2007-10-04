// ==================================================================
// @(#)bgp_topology.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 30/04/2007
// @lastdate 15/05/2007
// ==================================================================

#ifndef __CLI_BGP_TOPOLOGY_H__
#define __CLI_BGP_TOPOLOGY_H__

#include <libgds/cli.h>

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ cli_register_bgp_topology ]------------------------------
  int cli_register_bgp_topology(SCliCmds * pCmds);

#ifdef __cplusplus
}
#endif

#endif /* __CLI_BGP_TOPOLOGY_H__ */
