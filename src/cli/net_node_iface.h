// ==================================================================
// @(#)net_node_iface.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 14/02/2008
// $Id: net_node_iface.h,v 1.2 2008-04-11 11:03:06 bqu Exp $
// ==================================================================

#ifndef __CLI_NET_NODE_IFACE_H__
#define __CLI_NET_NODE_IFACE_H__

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ cli_register_net_node_add_iface ]------------------------
  int cli_register_net_node_add_iface(SCliCmds * pCmds);
  // -----[ cli_register_net_node_iface ]----------------------------
  int cli_register_net_node_iface(SCliCmds * pCmds);

#ifdef __cplusplus
extern "C" {
#endif

#endif /* __CLI_NET_NODE_IFACE_H__ */
