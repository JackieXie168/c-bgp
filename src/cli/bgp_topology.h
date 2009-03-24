// ==================================================================
// @(#)bgp_topology.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 30/04/2007
// $Id: bgp_topology.h,v 1.2 2009-03-24 15:58:43 bqu Exp $
// ==================================================================

#ifndef __CLI_BGP_TOPOLOGY_H__
#define __CLI_BGP_TOPOLOGY_H__

#include <libgds/cli.h>

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ cli_register_bgp_topology ]------------------------------
  void cli_register_bgp_topology(cli_cmd_t * parent);

#ifdef __cplusplus
}
#endif

#endif /* __CLI_BGP_TOPOLOGY_H__ */
