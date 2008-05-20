// ==================================================================
// @(#)iface.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 02/08/2003
// $Id: iface.h,v 1.5 2008-05-20 12:17:06 bqu Exp $
// ==================================================================

#ifndef __NET_IFACE_H__
#define __NET_IFACE_H__

#include <net/error.h>
#include <net/prefix.h>
#include <net/net_types.h>

typedef ip_pfx_t net_iface_id_t;
typedef enum {
  UNIDIR, BIDIR
} net_iface_dir_t;

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ net_iface_new ]------------------------------------------
  net_iface_t * net_iface_new(net_node_t * node, net_iface_type_t type);
  // -----[ net_iface_destroy ]--------------------------------------
  void net_iface_destroy(net_iface_t ** iface_ref);

  // -----[ net_iface_connect_iface ]--------------------------------
  int net_iface_connect_iface(net_iface_t * iface, net_iface_t * dst);
  // -----[ net_iface_connect_subnet ]-------------------------------
  int net_iface_connect_subnet(net_iface_t * iface, net_subnet_t * dst);
  // -----[ net_iface_disconnect ]-----------------------------------
  int net_iface_disconnect(net_iface_t * iface);

  // -----[ net_iface_dest_node ]------------------------------------
  net_node_t * net_iface_dest_node(net_iface_t * iface);
  // -----[ net_iface_dest_subnet ]----------------------------------
  net_subnet_t * net_iface_dest_subnet(net_iface_t * iface);
  // -----[ net_iface_dest_endpoint ]--------------------------------
  net_addr_t net_iface_dest_endpoint(net_iface_t * iface);

  // -----[ net_iface_has_address ]----------------------------------
  int net_iface_has_address(net_iface_t * iface, net_addr_t addr);
  // -----[ net_iface_has_prefix ]-----------------------------------
  int net_iface_has_prefix(net_iface_t * iface, ip_pfx_t prefix);
  // -----[ net_iface_src_address ]----------------------------------
  net_addr_t net_iface_src_address(net_iface_t * iface);
  // -----[ net_iface_dst_prefix ]-----------------------------------
  ip_pfx_t net_iface_dst_prefix(net_iface_t * iface);

  // -----[ net_iface_id ]-------------------------------------------
  net_iface_id_t net_iface_id(net_iface_t * iface);
  // -----[ net_iface_is_connected ]---------------------------------
  int net_iface_is_connected(net_iface_t * iface);

  // -----[ net_iface_factory ]--------------------------------------
  int net_iface_factory(net_node_t * node, net_iface_id_t id,
			net_iface_type_t type, net_iface_t ** iface_ref);

  // -----[ net_iface_recv ]-----------------------------------------
  int net_iface_recv(net_iface_t * iface, net_msg_t * msg);
  // -----[ net_iface_send ]-----------------------------------------
  int net_iface_send(net_iface_t * iface, net_addr_t next_hop,
		     net_msg_t * msg);

  ///////////////////////////////////////////////////////////////////
  // ATTRIBUTES
  ///////////////////////////////////////////////////////////////////

  // -----[ net_iface_get_metric ]-----------------------------------
  igp_weight_t net_iface_get_metric(net_iface_t * iface, net_tos_t tos);
  // -----[ net_iface_set_metric ]-----------------------------------
  int net_iface_set_metric(net_iface_t * iface, net_tos_t tos,
			   igp_weight_t weight, net_iface_dir_t eDir);
  // -----[ net_iface_is_enabled ]-----------------------------------
  int net_iface_is_enabled(net_iface_t * iface);
  // -----[ net_iface_set_enabled ]----------------------------------
  void net_iface_set_enabled(net_iface_t * iface, int enabled);
  // -----[ net_iface_get_delay ]------------------------------------
  net_link_delay_t net_iface_get_delay(net_iface_t * iface);
  // -----[ net_iface_set_delay ]------------------------------------
  int net_iface_set_delay(net_iface_t * iface, net_link_delay_t delay,
			  net_iface_dir_t dir);
  // -----[ net_iface_set_capacity ]-----------------------------------
  int net_iface_set_capacity(net_iface_t * iface, net_link_load_t capacity,
			     net_iface_dir_t dir);
  // -----[ net_iface_get_capacity ]---------------------------------
  net_link_load_t net_iface_get_capacity(net_iface_t * iface);
  // -----[ net_iface_set_load ]-------------------------------------
  void net_iface_set_load(net_iface_t * iface, net_link_load_t load);
  // -----[ net_iface_get_load ]-------------------------------------
  net_link_load_t net_iface_get_load(net_iface_t * iface);
  // -----[ net_iface_add_load ]-------------------------------------
  void net_iface_add_load(net_iface_t * iface, net_link_load_t inc_load);


  ///////////////////////////////////////////////////////////////////
  // DUMP
  ///////////////////////////////////////////////////////////////////

  // -----[ net_iface_dump_type ]------------------------------------
  void net_iface_dump_type(SLogStream * stream, net_iface_t * iface);
  // -----[ net_iface_dump_id ]--------------------------------------
  void net_iface_dump_id(SLogStream * stream, net_iface_t * iface);
  // -----[ net_iface_dump_dest ]------------------------------------
  void net_iface_dump_dest(SLogStream * stream, net_iface_t * iface);
  // -----[ net_iface_dump ]-----------------------------------------
  void net_iface_dump(SLogStream * stream, net_iface_t * iface,
		      int withDest);
  // -----[ net_iface_dumpf ]----------------------------------------
  void net_iface_dumpf(SLogStream * stream, net_iface_t * iface,
		       const char * format);


  ///////////////////////////////////////////////////////////////////
  // STRING TO TYPE / IDENTIFIER CONVERSION
  ///////////////////////////////////////////////////////////////////

  // -----[ net_iface_str2type ]-------------------------------------
  net_error_t net_iface_str2type(char * str, net_iface_type_t * ptr_type);
  // -----[ net_iface_type2str ]-------------------------------------
  const char * net_iface_type2str(net_iface_type_t type);
  // -----[ net_iface_str2id ]---------------------------------------
  net_error_t net_iface_str2id(char * str, net_iface_id_t * ptr_id);


  ///////////////////////////////////////////////////////////////////
  // INTERFACE IDENTIFIER UTILITIES
  ///////////////////////////////////////////////////////////////////

  static inline net_iface_id_t net_iface_id_rtr(net_node_t * node) {
    net_iface_id_t tID=
      { .tNetwork= node->addr,
	.uMaskLen= 32 };
    return tID;
  }

  static inline net_iface_id_t net_iface_id_addr(net_addr_t addr) {
    net_iface_id_t tID=
      { .tNetwork= addr,
	.uMaskLen= 32 };
    return tID;
  }

  static inline net_iface_id_t net_iface_id_pfx(net_addr_t network,
						uint8_t len) {
    net_iface_id_t tID=
      { .tNetwork= network,
	.uMaskLen= len };
    return tID;
  }

#ifdef __cplusplus
}
#endif

#endif /* __NET_IFACE_H__ */
