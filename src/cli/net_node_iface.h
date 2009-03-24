// ==================================================================
// @(#)net_node_iface.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 14/02/2008
// $Id: net_node_iface.h,v 1.3 2009-03-24 15:58:43 bqu Exp $
// ==================================================================

#ifndef __CLI_NET_NODE_IFACE_H__
#define __CLI_NET_NODE_IFACE_H__

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ cli_register_net_node_add_iface ]------------------------
  void cli_register_net_node_add_iface(cli_cmd_t * parent);
  // -----[ cli_register_net_node_iface ]----------------------------
  void cli_register_net_node_iface(cli_cmd_t * parent);

#ifdef __cplusplus
extern "C" {
#endif

#endif /* __CLI_NET_NODE_IFACE_H__ */
