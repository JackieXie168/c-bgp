// ==================================================================
// @(#)prefix.h
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be), Sebastien Tandel
// @date 01/12/2002
// @lastdate 15/09/2004
// ==================================================================

#ifndef __PREFIX_H__
#define __PREFIX_H__

#include <libgds/types.h>

#include <stdio.h>

typedef uint32_t net_addr_t;

typedef struct {
  net_addr_t tNetwork;
  uint8_t uMaskLen;
} SPrefix;

// ----- ip_address_to_string ----------------------------------------
void ip_address_to_string(char * pcAddr, net_addr_t tAddr);
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
// ----- ip_prefix_equals -------------------------------------------
extern int ip_prefix_equals(SPrefix sPrefix1, SPrefix sPrefix2);
// ----- ip_address_in_prefix ---------------------------------------
extern int ip_address_in_prefix(net_addr_t tAddr, SPrefix sPrefix);
// ----- ip_prefix_in_prefix ----------------------------------------
extern int ip_prefix_in_prefix(SPrefix sPrefix1, SPrefix sPrefix2);

#endif
