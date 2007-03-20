// ==================================================================
// @(#)prefix.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be), Sebastien Tandel
// @date 01/12/2002
// @lastdate 19/01/2007
// ==================================================================

#ifndef __PREFIX_H__
#define __PREFIX_H__

#include <libgds/log.h>
#include <libgds/types.h>

#include <stdio.h>

#define IPV4_TO_INT(A,B,C,D) (((((uint32_t)(A))*256 +(uint32_t)(B))*256 +(uint32_t)( C))*256 +(uint32_t)(D))

// ----- IPv4 address -----
typedef uint32_t net_addr_t;
typedef uint8_t  net_mask_t;
#define MAX_ADDR UINT32_MAX

// ----- IPv4 prefix -----
typedef struct {
  net_addr_t tNetwork;
  net_mask_t uMaskLen;
} SPrefix;

// ----- IPv4 destination -----
#define NET_DEST_INVALID 0
#define NET_DEST_ADDRESS 1
#define NET_DEST_PREFIX  2
#define NET_DEST_ANY     3
typedef union {
    SPrefix sPrefix;
    net_addr_t tAddr;  
} UNetDest;

typedef struct {
  uint8_t tType;
  UNetDest uDest;
} SNetDest;

#ifdef __cplusplus
extern "C" {
#endif

  // ----- ip_address_to_string ----------------------------------------
  void ip_address_to_string(char * pcAddr, net_addr_t tAddr);
  // ----- create_ip_prefix ----------------------------------------
  SPrefix * create_ip_prefix(net_addr_t tAddr, net_mask_t tMaskLen);
  // ----- ip_dotted_to_address ---------------------------------------
  net_addr_t ip_dotted_to_address(uint8_t uA, uint8_t uB,
				  uint8_t uC, uint8_t uD);
  // ----- ip_address_dump_string -------------------------------------
  char * ip_address_dump_string(net_addr_t tAddr);
  // ----- ip_address_dump --------------------------------------------
  void ip_address_dump(SLogStream * pStream, net_addr_t tAddr);
  // ----- ip_string_to_address ---------------------------------------
  int ip_string_to_address(char * pcString, char ** ppcEndPtr,
			   net_addr_t * ptAddr);
  // ----- uint32_to_prefix -------------------------------------------
  SPrefix uint32_to_prefix(net_addr_t tPrefix, net_mask_t tMaskLen);
  // ----- ip_prefix_dump_string ---------------------------------------------
  char * ip_prefix_dump_string(SPrefix sPrefix);
  // ----- ip_prefix_dump ---------------------------------------------
  void ip_prefix_dump(SLogStream * pStream, SPrefix sPrefix);
  // ----- ip_prefix_to_strin -----------------------------------------
  void ip_prefix_to_string(char * pcPrefix, SPrefix * pPrefix);
  // ----- ip_string_to_prefix ----------------------------------------
  int ip_string_to_prefix(char * pcString, char ** ppcEndPtr,
			  SPrefix * pPrefix);
  // ----- ip_string_to_dest ------------------------------------------
  int ip_string_to_dest(char * pcPrefix, SNetDest * psDest);
  // ----- ip_dest_dump -----------------------------------------------
  void ip_dest_dump(SLogStream * pLogStream, SNetDest sDest);
  // ----- ip_prefix_masked -------------------------------------------
  SPrefix ip_prefix_masked(const SPrefix * pPrefix);
  // ----- ip_prefix_equals -------------------------------------------
  int ip_prefix_equals(SPrefix sPrefix1, SPrefix sPrefix2);
  // ----- ip_prefixes_compare ---------------------------------------
  int ip_prefixes_compare(void * pItem1, void * pItem2,
			  unsigned int uEltSize);
  // ----- ip_address_in_prefix ---------------------------------------
  int ip_address_in_prefix(net_addr_t tAddr, SPrefix sPrefix);
  // ----- ip_prefix_in_prefix ----------------------------------------
  int ip_prefix_in_prefix(SPrefix sPrefix1, SPrefix sPrefix2);
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

#endif
