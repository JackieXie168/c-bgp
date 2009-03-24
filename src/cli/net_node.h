// ==================================================================
// @(#)net_node.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 15/05/2007
// $Id: net_node.h,v 1.3 2009-03-24 15:58:43 bqu Exp $
// ==================================================================

#ifndef __CLI_NET_NODE_H__
#define __CLI_NET_NODE_H__

#include <libgds/cli.h>

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ cli_register_net_node ]----------------------------------
  void cli_register_net_node(cli_cmd_t * parent);

#ifdef __cplusplus
}
#endif

#endif /* __CLI_NET_NODE_H__ */
