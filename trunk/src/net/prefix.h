// ==================================================================
// @(#)prefix.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 01/12/2002
// @lastdate 06/03/2008
// ==================================================================

#ifndef __PREFIX_H__
#define __PREFIX_H__

#include <libgds/log.h>
#include <libgds/types.h>

#include <stdio.h>

#include <net/ip.h>
#include <net/ip6.h>

typedef uint8_t net_afi_t;
#define NET_AFI_IP4 0
#define NET_AFI_IP6 1

#ifdef __cplusplus
extern "C" {
#endif

  // ----- ip_address_to_string ----------------------------------------
  int ip_address_to_string(net_addr_t tAddr, char * pcAddr, size_t tDstSize);
  // ----- create_ip_prefix ----------------------------------------
  SPrefix * create_ip_prefix(net_addr_t tAddr, net_mask_t tMaskLen);
  // ----- ip_address_dump --------------------------------------------
  void ip_address_dump(SLogStream * pStream, net_addr_t tAddr);
  // ----- ip_string_to_address ---------------------------------------
  int ip_string_to_address(char * pcString, char ** ppcEndPtr,
			   net_addr_t * ptAddr);
  // ----- uint32_to_prefix -------------------------------------------
  SPrefix uint32_to_prefix(net_addr_t tPrefix, net_mask_t tMaskLen);
  // ----- ip_prefix_dump ---------------------------------------------
  void ip_prefix_dump(SLogStream * pStream, SPrefix sPrefix);
  // ----- ip_prefix_to_string ----------------------------------------
  int ip_prefix_to_string(SPrefix * pPrefix, char * pcPrefix, size_t tDstSize);
  // ----- ip_string_to_prefix ----------------------------------------
  int ip_string_to_prefix(char * pcString, char ** ppcEndPtr,
			  SPrefix * pPrefix);
  // ----- ip_string_to_dest ------------------------------------------
  int ip_string_to_dest(char * pcPrefix, SNetDest * psDest);
  // ----- ip_dest_dump -----------------------------------------------
  void ip_dest_dump(SLogStream * pLogStream, SNetDest sDest);
  // -----[ ip_prefix_cmp ]--------------------------------------------
  int ip_prefix_cmp(SPrefix * pPrefix1, SPrefix * pPrefix2);
  // ----- ip_address_in_prefix ---------------------------------------
  int ip_address_in_prefix(net_addr_t tAddr, SPrefix sPrefix);
  // ----- ip_prefix_in_prefix ----------------------------------------
  int ip_prefix_in_prefix(SPrefix sPrefix1, SPrefix sPrefix2);
  // ----- ip_prefix_ge_prefix ----------------------------------------
  int ip_prefix_ge_prefix(SPrefix sPrefix1, SPrefix sPrefix2,
			  uint8_t uMaskLen);
  // ----- ip_prefix_le_prefix ----------------------------------------
  int ip_prefix_le_prefix(SPrefix sPrefix1, SPrefix sPrefix2,
			  uint8_t uMaskLen);
  // ----- ip_prefix_copy ---------------------------------------------
  SPrefix * ip_prefix_copy(SPrefix * pPrefix);
  // ----- ip_prefix_destroy ------------------------------------------
  void ip_prefix_destroy(SPrefix ** ppPrefix);
  // ----- ip_build_mask ----------------------------------------------
  net_addr_t ip_build_mask(uint8_t uMaskLen);
  // ----- ip_prefix_mask ---------------------------------------------
  void ip_prefix_mask(SPrefix * pPrefix);
  // ----- ip_prefix_to_dest ------------------------------------------
  SNetDest ip_prefix_to_dest(SPrefix sPrefix);
  // ----- ip_address_to_dest -----------------------------------------
  SNetDest ip_address_to_dest(net_addr_t tAddress);

#ifdef __cplusplus
}
#endif

#endif /* __IP_PREFIX_H__ */
