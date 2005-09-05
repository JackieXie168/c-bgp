// ==================================================================
// @(#)prefix.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be), Sebastien Tandel
// @date 01/12/2002
// @lastdate 05/08/2005
// ==================================================================

#ifndef __PREFIX_H__
#define __PREFIX_H__

#include <libgds/types.h>

#include <stdio.h>

#define IPV4_TO_INT(A,B,C,D) (((((uint32_t)(A))*256 +(uint32_t)(B))*256 +(uint32_t)( C))*256 +(uint32_t)(D))

// ----- IPv4 address -----
typedef uint32_t net_addr_t;

// ----- IPv4 prefix -----
typedef struct {
  net_addr_t tNetwork;
  uint8_t uMaskLen;
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

// ----- ip_address_to_string ----------------------------------------
void ip_address_to_string(char * pcAddr, net_addr_t tAddr);
// ----- create_ip_prefix ----------------------------------------
SPrefix * create_ip_prefix(net_addr_t tAddr, uint8_t uMaskLen);
// ----- ip_dotted_to_address ---------------------------------------
extern net_addr_t ip_dotted_to_address(uint8_t uA, uint8_t uB,
				       uint8_t uC, uint8_t uD);
// ----- ip_address_dump_string -------------------------------------
char * ip_address_dump_string(net_addr_t tAddr);
// ----- ip_address_dump --------------------------------------------
extern void ip_address_dump(FILE * pStream, net_addr_t tAddr);
// ----- ip_string_to_address ---------------------------------------
extern int ip_string_to_address(char * pcString, char ** ppcEndPtr,
				net_addr_t * ptAddr);
// ----- uint32_to_prefix -------------------------------------------
extern SPrefix uint32_to_prefix(net_addr_t tPrefix, uint8_t uMaskLen);
// ----- ip_prefix_dump_string ---------------------------------------------
char * ip_prefix_dump_string(SPrefix sPrefix);
// ----- ip_prefix_dump ---------------------------------------------
extern void ip_prefix_dump(FILE * pStream, SPrefix sPrefix);
// ----- ip_prefix_to_strin -----------------------------------------
void ip_prefix_to_string(char * pcPrefix, SPrefix * pPrefix);
// ----- ip_string_to_prefix ----------------------------------------
extern int ip_string_to_prefix(char * pcString, char ** ppcEndPtr,
			       SPrefix * pPrefix);
// ----- ip_string_to_dest ------------------------------------------
extern int ip_string_to_dest(char * pcPrefix, SNetDest * psDest);
// ----- ip_dest_dump -----------------------------------------------
extern void ip_dest_dump(FILE * pStream, SNetDest sDest);
// ----- ip_prefix_equals -------------------------------------------
extern int ip_prefix_equals(SPrefix sPrefix1, SPrefix sPrefix2);
// ----- ip_prefixes_destroy ----------------------------------------
extern void ip_prefixes_destroy(void ** ppItem);
 // ----- ip_prefixes_compare ---------------------------------------
extern int ip_prefixes_compare(void * pItem1, void * pItem2,
			       unsigned int uEltSize);
// ----- ip_address_in_prefix ---------------------------------------
extern int ip_address_in_prefix(net_addr_t tAddr, SPrefix sPrefix);
// ----- ip_prefix_in_prefix ----------------------------------------
extern int ip_prefix_in_prefix(SPrefix sPrefix1, SPrefix sPrefix2);
// ----- ip_prefix_copy ---------------------------------------------
extern SPrefix * ip_prefix_copy(SPrefix * pPrefix);
// ----- ip_prefix_destroy ------------------------------------------
extern void ip_prefix_destroy(SPrefix ** ppPrefix);
// ----- ip_build_mask ----------------------------------------------
extern net_addr_t ip_build_mask(uint8_t uMaskLen);
// ----- ip_prefix_mask ---------------------------------------------
extern void ip_prefix_mask(SPrefix * pPrefix);
// ----- ip_prefix_to_dest ------------------------------------------
extern SNetDest ip_prefix_to_dest(SPrefix sPrefix);
// ----- ip_address_to_dest -----------------------------------------
extern SNetDest ip_address_to_dest(net_addr_t tAddress);

#endif
