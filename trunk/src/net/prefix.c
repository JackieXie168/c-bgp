// ==================================================================
// @(#)prefix.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 01/11/2002
// @lastdate 06/03/2008
// ==================================================================

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <stdlib.h>
#include <libgds/log.h>
#include <libgds/memory.h>
#include <net/prefix.h>

#include <string.h>

// -----[ ip_address_to_string ]-------------------------------------
/**
 * Convert an IPv4 address to a string.
 *
 * Return value:
 *   -1    if buffer too small
 *   >= 0  number of characters written
 */
int ip_address_to_string(net_addr_t tAddr, char * pcAddr, size_t tDstSize)
{
  int iResult= snprintf(pcAddr, tDstSize, "%u.%u.%u.%u",
			(unsigned int) (tAddr >> 24),
			(unsigned int) (tAddr >> 16) & 255, 
			(unsigned int) (tAddr >> 8) & 255,
			(unsigned int) tAddr & 255);
  return ((iResult < 0) || (iResult >= tDstSize)) ? -1 : iResult;
}

// ----- ip_address_dump --------------------------------------------
/**
 *
 */
void ip_address_dump(SLogStream * pStream, net_addr_t tAddr)
{
  log_printf(pStream, "%u.%u.%u.%u",
	     (tAddr >> 24), (tAddr >> 16) & 255,
	     (tAddr >> 8) & 255, tAddr & 255);
}

// ----- ip_string_to_address ---------------------------------------
/**
 *
 */
int ip_string_to_address(char * pcString, char ** ppcEndPtr,
			 net_addr_t * ptAddr)
{
  unsigned long int ulDigit;
  uint8_t uNumDigits= 4;

  if (ptAddr == NULL)
    return -1;
  *ptAddr= 0;
  while (uNumDigits-- > 0) {
    ulDigit= strtoul(pcString, ppcEndPtr, 10);
    if ((pcString == *ppcEndPtr) || (ulDigit > 255) ||
	((uNumDigits > 0) && (**ppcEndPtr != '.')))
      return -1;
    pcString= *ppcEndPtr+1;
    *ptAddr= (*ptAddr << 8)+ulDigit;
  }
  return 0;
}

// -----[ ip_address_cmp ]-------------------------------------------
/**
 * Compare 2 IPv4 addresses.
 *
 * Return:
 *   1 if A1 > A2
 *   0 if A1 == A2
 *  -1 if A1 < A2
 */
static inline
int _ip_address_cmp(net_addr_t tAddr1, net_addr_t tAddr2)
{
  if (tAddr1 > tAddr2)
    return 1;
  if (tAddr1 < tAddr2)
    return -1;
  return 0;
}

// -----[ _ip_address_mask ]------------------------------------------
/**
 * Create a network mask based on a mask length.
 *
 * Pre: mask length in range [0,32]
 */
static inline
net_addr_t _ip_address_mask(net_mask_t tMaskLen)
{
  assert(tMaskLen <= 32);
  // Warning, shift on 32-bit int is only defined if operand in [0-31]
  // Thus, special case for mask-length value of 0
  if (tMaskLen == 0)
    return 0;
  return (0xffffffff << (32-tMaskLen));
}

// ----- uint32_to_prefix -------------------------------------------
/**
 *
 */
SPrefix uint32_to_prefix(net_addr_t tPrefix, net_mask_t tMaskLen)
{
  SPrefix sPrefix;
  sPrefix.tNetwork= tPrefix;
  sPrefix.uMaskLen= tMaskLen;
  return sPrefix;
}


// ----- create_ip_prefix -------------------------------------------
SPrefix * create_ip_prefix(net_addr_t tAddr, net_mask_t tMaskLen)
{
  SPrefix * pPrefix = (SPrefix *) MALLOC(sizeof(SPrefix));
  pPrefix->tNetwork= tAddr;
  pPrefix->uMaskLen= tMaskLen;
  return pPrefix;
}

// ----- ip_prefix_dump ---------------------------------------------
/**
 *
 */
void ip_prefix_dump(SLogStream * pStream, SPrefix sPrefix)
{
  log_printf(pStream, "%u.%u.%u.%u/%u",
	     sPrefix.tNetwork >> 24, (sPrefix.tNetwork >> 16) & 255,
	     (sPrefix.tNetwork >> 8) & 255, (sPrefix.tNetwork & 255),
	     sPrefix.uMaskLen);
}

// ----- ip_prefix_to_string ----------------------------------------
/**
 *
 *
 */
int ip_prefix_to_string(SPrefix * pPrefix, char * pcPrefix, size_t tDstSize)
{
  int iResult= snprintf(pcPrefix, tDstSize, "%u.%u.%u.%u/%u", 
			(unsigned int) pPrefix->tNetwork >> 24,
			(unsigned int) (pPrefix->tNetwork >> 16) & 255,
			(unsigned int) (pPrefix->tNetwork >> 8) & 255,
			(unsigned int) pPrefix->tNetwork & 255,
			pPrefix->uMaskLen);
  return ((iResult < 0) || (iResult >= tDstSize)) ? -1 : iResult;
}

// ----- ip_string_to_prefix ----------------------------------------
/**
 *
 */
int ip_string_to_prefix(char * pcString, char ** ppcEndPtr,
			SPrefix * pPrefix)
{
  unsigned long int ulDigit;
  uint8_t uNumDigits= 4;

  if (pPrefix == NULL)
    return -1;
  pPrefix->tNetwork= 0;
  while (uNumDigits-- > 0) {
    ulDigit= strtoul(pcString, ppcEndPtr, 10);
    if ((ulDigit > 255) || (pcString == *ppcEndPtr))
      return -1;
    pcString= *ppcEndPtr+1;
    pPrefix->tNetwork= (pPrefix->tNetwork << 8)+ulDigit;
    if (**ppcEndPtr == '/')
      break;
    if (**ppcEndPtr != '.')
      return -1;
  }
  if (**ppcEndPtr != '/')
    return -1;
  while (uNumDigits-- > 0)
    pPrefix->tNetwork<<= 8;
  ulDigit= strtoul(pcString, ppcEndPtr, 10);
  if (ulDigit > 32)
    return -1;
  pPrefix->uMaskLen= ulDigit;
  return 0;
}

// ----- ip_string_to_dest ------------------------------------------
/**
 * This function parses a string that contains the description of a
 * destination. The destination can be specified as an asterisk ('*'),
 * an IP prefix ('x.y/z') or an IP address ('a.b.c.d').
 *
 * If the destination is an asterisk, the function returns a
 * destination of type NET_DEST_ANY.
 *
 * If the destination is an IP prefix, the function returns a
 * destination of type NET_DEST_PREFIX.
 *
 * If the destination is an IP address, the function returns a
 * destination whose type is NET_DEST_ADDRESS.
 */
int ip_string_to_dest(char * pcPrefix, SNetDest * psDest)
{
  char * pcEndChar;

  if (!strcmp(pcPrefix, "*")) {
    psDest->tType= NET_DEST_ANY;
    psDest->uDest.sPrefix.uMaskLen= 0;
  } else if (!ip_string_to_prefix(pcPrefix, &pcEndChar,
				  &psDest->uDest.sPrefix) &&
	     (*pcEndChar == 0)) {
    psDest->tType= NET_DEST_PREFIX;
  } else if (!ip_string_to_address(pcPrefix, &pcEndChar,
				   &psDest->uDest.sPrefix.tNetwork) &&
	     (*pcEndChar == 0)) {
    psDest->tType= NET_DEST_ADDRESS;
  } else {
    psDest->tType= NET_DEST_INVALID;
    return -1;
  }
  return 0;
}

//---------- ip_prefix_to_dest ----------------------------------------------
SNetDest ip_prefix_to_dest(SPrefix sPrefix)
{
  SNetDest sDest;
  if (sPrefix.uMaskLen == 32){
    sDest.tType = NET_DEST_ADDRESS;
    sDest.uDest.tAddr = sPrefix.tNetwork;
  }
  else {
    sDest.tType = NET_DEST_PREFIX;
    sDest.uDest.sPrefix = sPrefix;
  }
  return sDest;
}

//---------- ip_address_to_dest ----------------------------------------------
SNetDest ip_address_to_dest(net_addr_t tAddress)
{
  SNetDest sDest;
  sDest.tType = NET_DEST_ADDRESS;
  sDest.uDest.tAddr = tAddress;
  return sDest;
}


// ----- ip_dest_dump -----------------------------------------------
/**
 *
 */
void ip_dest_dump(SLogStream * pStream, SNetDest sDest)
{
  switch (sDest.tType) {
  case NET_DEST_ADDRESS:
    ip_address_dump(pStream, sDest.uDest.tAddr);
    break;
  case NET_DEST_PREFIX:
    ip_prefix_dump(pStream, sDest.uDest.sPrefix);
    break;
  case NET_DEST_ANY:
    log_printf(pStream, "*");
    break;
  default:
    log_printf(pStream, "???");
  }
}

// -----[ ip_prefix_cmp ]--------------------------------------------
/**
 * Compare two prefixes. The function makes sure that the network
 * part is masked before the comparison is done.
 *
 * Return value:
 *   -1  if P1 < P2
 *   0   if P1 == P2
 *   1   if P1 > P2
 */
int ip_prefix_cmp(SPrefix * pPrefix1, SPrefix * pPrefix2)
{
  net_addr_t tMask;

  // Pointers are equal ?
  if (pPrefix1 == pPrefix2)
    return 0;
  
  // Compare mask length
  if (pPrefix1->uMaskLen < pPrefix2->uMaskLen) 
    return -1;
  else if (pPrefix1->uMaskLen > pPrefix2->uMaskLen)
    return  1;

  // Compare masked network part
  tMask= _ip_address_mask(pPrefix1->uMaskLen);
  return _ip_address_cmp(pPrefix1->tNetwork & tMask,
			 pPrefix2->tNetwork & tMask);
}

// ----- ip_address_in_prefix ---------------------------------------
/**
 * Test if the address is in the prefix (i.e. if the address matches
 * the prefix).
 *
 * Return value:
 *   != 0  if address is in prefix
 *   0     otherwise
 */
int ip_address_in_prefix(net_addr_t tAddr, SPrefix sPrefix)
{
  assert(sPrefix.uMaskLen <= 32);

  // Warning, shift on 32-bit int is only defined if operand in [0-31]
  // Thus, special case for 0 mask-length (match everything)
  if (sPrefix.uMaskLen == 0)
    return 1;

  return ((tAddr >> (32-sPrefix.uMaskLen)) ==
	  (sPrefix.tNetwork >> (32-sPrefix.uMaskLen)));
}

// ----- ip_prefix_in_prefix ----------------------------------------
/**
 * Test if P1 is more specific than P2.
 *
 * Return value:
 *   != 0  if P1 is in P2
 *   0     otherwise
 */
int ip_prefix_in_prefix(SPrefix sPrefix1, SPrefix sPrefix2)
{
  assert(sPrefix2.uMaskLen <= 32);

  // Warning, shift on 32-bit int is only defined if operand in [0-31]
  // Thus, special case for 0 mask-length (match everything)
  if (sPrefix2.uMaskLen == 0)
    return 1;

  // P1.masklen must be >= P2.masklen
  if (sPrefix1.uMaskLen < sPrefix2.uMaskLen)
    return 0;

  // Compare bits masked with less specific (P2)
  // Warning, shift on 32-bit int is only defined if operand in [0-31]
  return ((sPrefix1.tNetwork >> (32-sPrefix2.uMaskLen)) ==
          (sPrefix2.tNetwork >> (32-sPrefix2.uMaskLen)));
}

// ----- ip_prefix_ge_prefix ----------------------------------------
/**
 * Test if P1 matches P2 and its prefix length is greater than or
 * equal to L.
 */
int ip_prefix_ge_prefix(SPrefix sPrefix1, SPrefix sPrefix2,
			uint8_t uMaskLen)
{
  if (!ip_prefix_in_prefix(sPrefix1, sPrefix2))
    return 0;

  return (sPrefix1.uMaskLen >= uMaskLen);
}

// ----- ip_prefix_le_prefix ----------------------------------------
/**
 * Test if P1 is less or equal than P2.
 */
int ip_prefix_le_prefix(SPrefix sPrefix1, SPrefix sPrefix2,
			uint8_t uMaskLen)
{
  if (!ip_prefix_in_prefix(sPrefix1, sPrefix2))
    return 0;

  return (sPrefix1.uMaskLen <= uMaskLen);
}

// ----- ip_prefix_copy ---------------------------------------------
/**
 * Make a copy of the given IP prefix.
 */
SPrefix * ip_prefix_copy(SPrefix * pPrefix)
{
  SPrefix * pPrefixCopy= (SPrefix *) MALLOC(sizeof(SPrefix));
  memcpy(pPrefixCopy, pPrefix, sizeof(SPrefix));
  return pPrefixCopy;
}

// ----- ip_prefix_destroy ------------------------------------------
/**
 *
 */
void ip_prefix_destroy(SPrefix ** ppPrefix)
{
  if (*ppPrefix != NULL) {
    FREE(*ppPrefix);
    *ppPrefix= NULL;
  }
}

// ----- ip_prefix_mask ---------------------------------------------
/**
 * This function masks the remaining bits of the prefix's address.
 */
void ip_prefix_mask(SPrefix * pPrefix)
{
  pPrefix->tNetwork&= _ip_address_mask(pPrefix->uMaskLen);
}

