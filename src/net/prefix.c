// ==================================================================
// @(#)prefix.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @author Sebastien Tandel (standel@info.ucl.ac.be)
// @date 01/11/2002
// @lastdate 07/04/2005
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

// ----- ip_dotted_to_address ---------------------------------------
/**
 *
 */
net_addr_t ip_dotted_to_address(uint8_t uA, uint8_t uB,
				uint8_t uC, uint8_t uD)
{
  return (((((((net_addr_t) uA) << 8) + uB) << 8) + uC) << 8) + uD;
}

// ----- ip_address_to_string ----------------------------------------
/**
 *
 *
 */
void ip_address_to_string(char * pcAddr, net_addr_t tAddr)
{
  sprintf(pcAddr, "%u.%u.%u.%u", (tAddr >> 24), (tAddr >> 16) & 255, 
	  (tAddr >> 8) & 255, tAddr & 255);
}

// ----- ip_address_dump --------------------------------------------
/**
 *
 */
char * ip_address_dump_string(net_addr_t tAddr)
{
  char * cAddress = MALLOC(16);
  sprintf(cAddress, "%u.%u.%u.%u", (tAddr >> 24), (tAddr >> 16) & 255,
	  (tAddr >> 8) & 255, tAddr & 255);
  return cAddress;
}

// ----- ip_address_dump --------------------------------------------
/**
 *
 */
void ip_address_dump(FILE * pStream, net_addr_t tAddr)
{
  fprintf(pStream, "%u.%u.%u.%u", (tAddr >> 24), (tAddr >> 16) & 255,
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

// ----- uint32_to_prefix -------------------------------------------
/**
 *
 */
SPrefix uint32_to_prefix(net_addr_t tPrefix, uint8_t uMaskLen)
{
  SPrefix sPrefix;
  sPrefix.tNetwork= tPrefix;
  sPrefix.uMaskLen= uMaskLen;
  return sPrefix;
}

// ----- ip_prefix_dump_string ---------------------------------------------
/**
 *
 */
char * ip_prefix_dump_string(SPrefix sPrefix)
{
  char * cPrefix = MALLOC(20);
  
  sprintf(cPrefix, "%u.%u.%u.%u/%u",
	  sPrefix.tNetwork >> 24, (sPrefix.tNetwork >> 16) & 255,
	  (sPrefix.tNetwork >> 8) & 255, (sPrefix.tNetwork & 255),
	  sPrefix.uMaskLen);
  return cPrefix;
}
// ----- ip_prefix_dump ---------------------------------------------
/**
 *
 */
void ip_prefix_dump(FILE * pStream, SPrefix sPrefix)
{
  fprintf(pStream, "%u.%u.%u.%u/%u",
	  sPrefix.tNetwork >> 24, (sPrefix.tNetwork >> 16) & 255,
	  (sPrefix.tNetwork >> 8) & 255, (sPrefix.tNetwork & 255),
	  sPrefix.uMaskLen);
}

// ----- ip_prefix_to_string ----------------------------------------
/**
 *
 *
 */
void ip_prefix_to_string(char * pcPrefix, SPrefix * pPrefix)
{
  sprintf(pcPrefix, "%u.%u.%u.%u/%u", 
      pPrefix->tNetwork >> 24, (pPrefix->tNetwork >> 16) & 255,
      (pPrefix->tNetwork >> 8) & 255, pPrefix->tNetwork & 255,
      pPrefix->uMaskLen);
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

  } else if (!ip_string_to_prefix(pcPrefix, &pcEndChar, &psDest->uDest.sPrefix) &&
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

// ----- ip_dest_dump -----------------------------------------------
/**
 *
 */
void ip_dest_dump(FILE * pStream, SNetDest sDest)
{
  switch (sDest.tType) {
  case NET_DEST_ADDRESS:
    ip_address_dump(pStream, sDest.uDest.tAddr);
    break;
  case NET_DEST_PREFIX:
    ip_prefix_dump(pStream, sDest.uDest.sPrefix);
    break;
  case NET_DEST_ANY:
    fprintf(pStream, "*");
    break;
  default:
    fprintf(pStream, "???");
  }
}

// ----- ip_prefix_equals -------------------------------------------
/**
 *
 */
int ip_prefix_equals(SPrefix sPrefix1, SPrefix sPrefix2)
{
  return (sPrefix1.tNetwork == sPrefix2.tNetwork) &&
    (sPrefix1.uMaskLen == sPrefix2.uMaskLen);
}

// ----- ip_address_in_prefix ---------------------------------------
/**
 *
 */
int ip_address_in_prefix(net_addr_t tAddr, SPrefix sPrefix)
{
  return ((tAddr >> (32-sPrefix.uMaskLen)) ==
	  (sPrefix.tNetwork >> (32-sPrefix.uMaskLen)));
}

// ----- ip_prefix_in_prefix ----------------------------------------
/**
 *
 */
int ip_prefix_in_prefix(SPrefix sPrefix1, SPrefix sPrefix2)
{
  if (sPrefix1.uMaskLen < sPrefix2.uMaskLen)
    return 0;

  return ((sPrefix1.tNetwork & sPrefix1.uMaskLen) ==
	  (sPrefix2.tNetwork & sPrefix1.uMaskLen));
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

/////////////////////////////////////////////////////////////////////
// TEST
/////////////////////////////////////////////////////////////////////

// ----- test_prefix ------------------------------------------------
void test_prefix()
{
  char * pcEndPtr;
  net_addr_t tAddr;
  SPrefix sPrefix;

  assert(!ip_string_to_address("192.168.0.1", &pcEndPtr, &tAddr));
  assert(*pcEndPtr == 0);
  ip_address_dump(stdout, tAddr);
  assert(!ip_string_to_prefix("192.168.0.0/24", &pcEndPtr, &sPrefix));
  assert(*pcEndPtr == 0);
  ip_prefix_dump(stdout, sPrefix);
  printf("\n");
  assert(!ip_string_to_prefix("192.168.0/24", &pcEndPtr, &sPrefix));
  assert(*pcEndPtr == 0);
  ip_prefix_dump(stdout, sPrefix);
  printf("\n");

  exit(0);
}
