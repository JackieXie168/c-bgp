// ==================================================================
// @(#)prefix.c
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be), Sebastien Tandel
// @date 01/11/2002
// @lastdate 24/02/2004
// ==================================================================

#include <assert.h>
#include <stdlib.h>
#include <libgds/log.h>
#include <libgds/memory.h>
#include <net/prefix.h>

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
  return ((tAddr & sPrefix.uMaskLen) ==
	  (sPrefix.tNetwork & sPrefix.uMaskLen));
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