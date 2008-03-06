// ==================================================================
// @(#)iface.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 02/08/2003
// @lastdate 05/03/2008
// ==================================================================

#ifndef __NET_IFACE_H__
#define __NET_IFACE_H__

#include <net/error.h>
#include <net/prefix.h>
#include <net/net_types.h>

typedef SPrefix net_iface_id_t;
typedef enum {
  UNIDIR, BIDIR
} EIfaceDir;

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ net_iface_new ]------------------------------------------
  SNetIface * net_iface_new(SNetNode * pNode, net_iface_type_t tType);
  // -----[ net_iface_destroy ]--------------------------------------
  void net_iface_destroy(SNetIface ** ppIface);

  // -----[ net_iface_connect_iface ]--------------------------------
  int net_iface_connect_iface(SNetIface * pIface, SNetIface * pDst);
  // -----[ net_iface_connect_subnet ]-------------------------------
  int net_iface_connect_subnet(SNetIface * pIface, SNetSubnet * pDst);
  // -----[ net_iface_disconnect ]-----------------------------------
  int net_iface_disconnect(SNetIface * pIface);

  // -----[ net_iface_dest_node ]------------------------------------
  SNetNode * net_iface_dest_node(SNetIface * pIface);
  // -----[ net_iface_dest_subnet ]----------------------------------
  SNetSubnet * net_iface_dest_subnet(SNetIface * pIface);
  // -----[ net_iface_dest_endpoint ]--------------------------------
  net_addr_t net_iface_dest_endpoint(SNetIface * pIface);

  // -----[ net_iface_has_address ]----------------------------------
  int net_iface_has_address(SNetIface * pIface, net_addr_t tAddr);
  // -----[ net_iface_has_prefix ]-----------------------------------
  int net_iface_has_prefix(SNetIface * pIface, SPrefix sPrefix);
  // -----[ net_iface_src_address ]----------------------------------
  net_addr_t net_iface_src_address(SNetIface * pIface);
  // -----[ net_iface_dst_prefix ]-----------------------------------
  SPrefix net_iface_dst_prefix(SNetIface * pIface);

  // -----[ net_iface_id ]-------------------------------------------
  net_iface_id_t net_iface_id(SNetIface * pIface);
  // -----[ net_iface_is_connected ]---------------------------------
  int net_iface_is_connected(SNetIface * pIface);

  // -----[ net_iface_factory ]--------------------------------------
  int net_iface_factory(SNetNode * pNode, net_iface_id_t sID,
			net_iface_type_t tType, SNetIface ** ppIface);

  // -----[ net_iface_recv ]-----------------------------------------
  int net_iface_recv(SNetIface * pIface, SNetMessage * pMsg);
  // -----[ net_iface_send ]-----------------------------------------
  int net_iface_send(SNetIface * pIface, SNetIface ** ppDstIface,
		     net_addr_t tNextHop, SNetMessage ** ppMsg);

  ///////////////////////////////////////////////////////////////////
  // ATTRIBUTES
  ///////////////////////////////////////////////////////////////////

  // -----[ net_iface_get_metric ]-----------------------------------
  net_igp_weight_t net_iface_get_metric(SNetIface * pIface, net_tos_t tTOS);
  // -----[ net_iface_set_metric ]-----------------------------------
  int net_iface_set_metric(SNetIface * pIface, net_tos_t tTOS,
			   net_igp_weight_t tWeight, EIfaceDir eDir);
  // -----[ net_iface_is_enabled ]-----------------------------------
  int net_iface_is_enabled(SNetIface * pIface);
  // -----[ net_iface_set_enabled ]----------------------------------
  void net_iface_set_enabled(SNetIface * pIface, int iEnabled);
  // -----[ net_iface_get_delay ]------------------------------------
  net_link_delay_t net_iface_get_delay(SNetIface * pIface);
  // -----[ net_iface_set_delay ]------------------------------------
  int net_iface_set_delay(SNetIface * pIface, net_link_delay_t tDelay,
			  EIfaceDir eDir);
  // -----[ net_iface_set_capacity ]-----------------------------------
  int net_iface_set_capacity(SNetIface * pIface, net_link_load_t tCapacity,
			     EIfaceDir eDir);
  // -----[ net_iface_get_capacity ]---------------------------------
  net_link_load_t net_iface_get_capacity(SNetIface * pIface);
  // -----[ net_iface_set_load ]-------------------------------------
  void net_iface_set_load(SNetIface * pIface, net_link_load_t tLoad);
  // -----[ net_iface_get_load ]-------------------------------------
  net_link_load_t net_iface_get_load(SNetIface * pIface);
  // -----[ net_iface_add_load ]-------------------------------------
  void net_iface_add_load(SNetIface * pIface, net_link_load_t tIncLoad);


  ///////////////////////////////////////////////////////////////////
  // DUMP
  ///////////////////////////////////////////////////////////////////

  // -----[ net_iface_dump_type ]------------------------------------
  void net_iface_dump_type(SLogStream * pStream, SNetIface * pIface);
  // -----[ net_iface_dump_id ]--------------------------------------
  void net_iface_dump_id(SLogStream * pStream, SNetIface * pIface);
  // -----[ net_iface_dump_dest ]------------------------------------
  void net_iface_dump_dest(SLogStream * pStream, SNetIface * pIface);
  // -----[ net_iface_dump ]-----------------------------------------
  void net_iface_dump(SLogStream * pStream, SNetIface * pIface,
		      int withDest);
  // -----[ net_iface_dumpf ]----------------------------------------
  void net_iface_dumpf(SLogStream * pStream, SNetIface * pIface,
		       const char * pcFormat);


  ///////////////////////////////////////////////////////////////////
  // STRING TO TYPE / IDENTIFIER CONVERSION
  ///////////////////////////////////////////////////////////////////

  // -----[ net_iface_str2type ]-------------------------------------
  net_iface_type_t net_iface_str2type(char * pcType);
  // -----[ net_iface_str2id ]---------------------------------------
  int net_iface_str2id(char * pcID, net_iface_id_t * ptID);


  ///////////////////////////////////////////////////////////////////
  // INTERFACE IDENTIFIER UTILITIES
  ///////////////////////////////////////////////////////////////////

  static inline net_iface_id_t net_iface_id_rtr(SNetNode * pNode) {
    net_iface_id_t tID=
      { .tNetwork= pNode->tAddr,
	.uMaskLen= 32 };
    return tID;
  }

  static inline net_iface_id_t net_iface_id_addr(net_addr_t tAddr) {
    net_iface_id_t tID=
      { .tNetwork= tAddr,
	.uMaskLen= 32 };
    return tID;
  }

  static inline net_iface_id_t net_iface_id_pfx(net_addr_t tNetwork, uint8_t tMaskLen) {
    net_iface_id_t tID=
      { .tNetwork= tNetwork,
	.uMaskLen= tMaskLen };
    return tID;
  }

#ifdef __cplusplus
}
#endif

#endif /* __NET_IFACE_H__ */
